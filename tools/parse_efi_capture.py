#!/usr/bin/env python3
"""Parse the text output from GpuReadOnlyProbe into stable JSON."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


HEX_RE = re.compile(r"^[0-9a-fA-F]+$")


class CaptureError(ValueError):
    pass


def parse_hex(value: str, label: str, bits: int) -> int:
    if not HEX_RE.fullmatch(value):
        raise CaptureError(f"{label}: expected hexadecimal digits")
    parsed = int(value, 16)
    if parsed >= (1 << bits):
        raise CaptureError(f"{label}: value exceeds {bits} bits")
    return parsed


def parse_decimal(value: str, label: str, maximum: int) -> int:
    if not value.isdecimal():
        raise CaptureError(f"{label}: expected decimal digits")
    parsed = int(value, 10)
    if parsed > maximum:
        raise CaptureError(f"{label}: value exceeds allowed range")
    return parsed


def parse_bdf(fields: list[str], offset: int) -> dict[str, int]:
    segment = parse_hex(fields[offset], "segment", 16)
    bus = parse_hex(fields[offset + 1], "bus", 8)
    device = parse_hex(fields[offset + 2], "device", 5)
    function = parse_hex(fields[offset + 3], "function", 3)
    return {
        "segment": segment,
        "bus": bus,
        "device": device,
        "function": function,
    }


def parse_capture(text: str) -> dict[str, Any]:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if not lines or lines[0] != "GPU_UNLOCK_DIAG|1":
        raise CaptureError("missing GPU_UNLOCK_DIAG version 1 header")

    secure_boot: str | None = None
    devices: list[dict[str, Any]] = []
    bridges: list[dict[str, Any]] = []
    bars: list[dict[str, Any]] = []
    errors: list[dict[str, str]] = []
    seen_locations: set[tuple[int, int, int, int]] = set()
    ended = False

    for line_number, line in enumerate(lines[1:], start=2):
        fields = line.split("|")
        kind = fields[0]

        if ended:
            # The EFI app prints a human prompt after the machine-readable END row.
            continue
        if kind == "NOTE":
            continue
        if kind == "SECURE_BOOT":
            if len(fields) != 2 or secure_boot is not None:
                raise CaptureError(f"line {line_number}: malformed or duplicate SECURE_BOOT row")
            if fields[1] not in {"enabled", "disabled", "unknown"}:
                raise CaptureError(f"line {line_number}: invalid Secure Boot value")
            secure_boot = fields[1]
        elif kind == "DEVICE":
            if len(fields) != 15:
                raise CaptureError(f"line {line_number}: DEVICE row must have 14 values")
            bdf = parse_bdf(fields, 1)
            location = (bdf["segment"], bdf["bus"], bdf["device"], bdf["function"])
            if location in seen_locations:
                raise CaptureError(f"line {line_number}: duplicate PCI location")
            seen_locations.add(location)
            if fields[13] not in {"yes", "no"}:
                raise CaptureError(f"line {line_number}: invalid same-handle GOP value")
            devices.append(
                {
                    **bdf,
                    "vendor_id": parse_hex(fields[5], "vendor_id", 16),
                    "device_id": parse_hex(fields[6], "device_id", 16),
                    "base_class": parse_hex(fields[7], "base_class", 8),
                    "subclass": parse_hex(fields[8], "subclass", 8),
                    "programming_interface": parse_hex(fields[9], "programming_interface", 8),
                    "revision": parse_hex(fields[10], "revision", 8),
                    "command": parse_hex(fields[11], "command", 16),
                    "status": parse_hex(fields[12], "status", 16),
                    "gop_on_same_handle": fields[13] == "yes",
                    "header_type": parse_hex(fields[14], "header_type", 8),
                }
            )
        elif kind == "BRIDGE":
            if len(fields) != 8:
                raise CaptureError(f"line {line_number}: BRIDGE row must have 7 values")
            bridges.append(
                {
                    **parse_bdf(fields, 1),
                    "primary_bus": parse_hex(fields[5], "primary_bus", 8),
                    "secondary_bus": parse_hex(fields[6], "secondary_bus", 8),
                    "subordinate_bus": parse_hex(fields[7], "subordinate_bus", 8),
                }
            )
        elif kind == "BAR":
            if len(fields) != 10:
                raise CaptureError(f"line {line_number}: BAR row must have 9 values")
            if fields[6] not in {"MEM", "IO", "BUS", "OTHER"}:
                raise CaptureError(f"line {line_number}: invalid BAR resource type")
            bars.append(
                {
                    **parse_bdf(fields, 1),
                    "bar_index": parse_decimal(fields[5], "bar_index", 5),
                    "resource_type": fields[6],
                    "base": parse_hex(fields[7], "bar_base", 64),
                    "length": parse_hex(fields[8], "bar_length", 64),
                    "granularity_bits": parse_decimal(fields[9], "granularity", 64),
                }
            )
        elif kind == "ERROR":
            errors.append(
                {
                    "line": str(line_number),
                    "record": line,
                }
            )
        elif kind == "END":
            if fields != ["END", "1"]:
                raise CaptureError(f"line {line_number}: malformed END row")
            ended = True
        else:
            raise CaptureError(f"line {line_number}: unknown record type {kind!r}")

    if not ended:
        raise CaptureError("missing END|1 row")
    if secure_boot is None:
        raise CaptureError("missing SECURE_BOOT row")

    return {
        "schema_version": 1,
        "capture_format": "gpu-unlock-efi-diagnostic-v1",
        "capture_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
        "secure_boot": secure_boot,
        "devices": devices,
        "bridges": bridges,
        "bar_resources": bars,
        "collection_errors": errors,
        "assessment": {
            "status": "not-assessed",
            "can_unlock": False,
            "reason": "An EFI observation capture cannot establish compatibility or authorize a hardware change.",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="text capture copied from the EFI console")
    parser.add_argument("-o", "--output", type=Path, help="write JSON to this local path")
    args = parser.parse_args()

    try:
        text = args.capture.read_text(encoding="utf-8-sig")
        result = parse_capture(text)
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            sys.stdout.write(rendered)
    except (OSError, CaptureError) as exc:
        print(f"capture parse failed: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
