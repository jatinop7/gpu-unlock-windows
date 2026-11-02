#!/usr/bin/env python3
"""Compare Windows and EFI observations; never authorizes or performs writes."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


IDENTITY_FIELDS = (
    "vendor_id",
    "device_id",
    "subsystem_vendor_id",
    "subsystem_device_id",
    "system_product_name",
    "bios_version",
    "vbios_version",
)


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8-sig") as stream:
        return json.load(stream)


def normalize_hex(value: Any, digits: int = 4) -> str | None:
    if value is None:
        return None
    if isinstance(value, int) and not isinstance(value, bool):
        if value < 0:
            return None
        return f"{value:0{digits}x}"
    text = str(value).strip().lower()
    if text.startswith("0x"):
        text = text[2:]
    if not text or any(char not in "0123456789abcdef" for char in text):
        return None
    return text.zfill(digits)


def current_identities(inventory: dict[str, Any], capture: dict[str, Any]) -> list[dict[str, Any]]:
    hardware = inventory.get("hardware", {})
    system = hardware.get("system", {})
    adapters = hardware.get("display_adapters", [])
    nvidia_adapters = []
    for adapter in adapters if isinstance(adapters, list) else []:
        pci = adapter.get("pci") if isinstance(adapter, dict) else None
        if not isinstance(pci, dict) or normalize_hex(pci.get("vendor_id")) != "10de":
            continue
        nvidia_adapters.append(
            {
                "vendor_id": normalize_hex(pci.get("vendor_id")),
                "device_id": normalize_hex(pci.get("device_id")),
                "subsystem_vendor_id": normalize_hex(pci.get("subsystem_vendor_id")),
                "subsystem_device_id": normalize_hex(pci.get("subsystem_device_id")),
                "system_product_name": system.get("SystemProductName"),
                "bios_version": system.get("BIOSVersion"),
                "vbios_version": None,
            }
        )

    smi_rows = hardware.get("nvidia_smi", [])
    if len(nvidia_adapters) == 1 and isinstance(smi_rows, list) and len(smi_rows) == 1:
        nvidia_adapters[0]["vbios_version"] = smi_rows[0].get("vbios_version")

    efi_devices = capture.get("devices", [])
    efi_targets = [
        device
        for device in efi_devices
        if isinstance(device, dict)
        and normalize_hex(device.get("vendor_id")) == "10de"
        and device.get("base_class") == 0x03
    ]
    if len(nvidia_adapters) != 1 or len(efi_targets) != 1:
        return []

    windows = nvidia_adapters[0]
    efi = efi_targets[0]
    if windows["device_id"] != normalize_hex(efi.get("device_id")):
        return []
    if windows["vendor_id"] != normalize_hex(efi.get("vendor_id")):
        return []
    return [windows]


def gate(
    inventory: dict[str, Any],
    capture: dict[str, Any],
    registry: dict[str, Any],
) -> dict[str, Any]:
    reasons: list[str] = []
    candidates = current_identities(inventory, capture)
    if len(candidates) != 1:
        reasons.append("Windows and EFI observations do not identify exactly one matching NVIDIA display controller.")

    profiles = registry.get("profiles")
    if not isinstance(profiles, list):
        profiles = []
        reasons.append("Profile registry is malformed.")
    if not profiles:
        reasons.append("No exact machine profile has been reviewed and admitted.")

    matching_profile: str | None = None
    if len(candidates) == 1:
        current = candidates[0]
        for profile in profiles:
            if not isinstance(profile, dict):
                continue
            identity = profile.get("identity")
            evidence = profile.get("evidence")
            if not isinstance(identity, dict) or not isinstance(evidence, dict):
                continue
            if profile.get("status") != "hardware-validated":
                continue
            if profile.get("write_enabled") is not True:
                continue
            if evidence.get("recovery_documented") is not True:
                continue
            if evidence.get("independent_recovery_available") is not True:
                continue
            if evidence.get("recovery_tested_on_exact_machine") is not True:
                continue
            if evidence.get("human_reviewed") is not True:
                continue
            if any(identity.get(field) != current.get(field) or current.get(field) is None for field in IDENTITY_FIELDS):
                continue
            matching_profile = profile.get("profile_id")
            break
    if matching_profile is None and profiles:
        reasons.append("No reviewed profile exactly matches the current machine identity.")

    return {
        "schema_version": 1,
        "status": "not-assessed",
        "matched_profile": matching_profile,
        "can_unlock": False,
        "write_permitted": False,
        "write_path_present": True,
        "reasons": reasons + [
            "This offline gate never grants write authority; the EFI executor requires a separately reviewed compiled profile."
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inventory", type=Path)
    parser.add_argument("efi_capture", type=Path)
    parser.add_argument(
        "--profiles",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "profiles" / "registry.json",
    )
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    try:
        result = gate(load_json(args.inventory), load_json(args.efi_capture), load_json(args.profiles))
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            sys.stdout.write(rendered)
    except (OSError, json.JSONDecodeError, TypeError) as exc:
        print(f"profile assessment failed closed: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
