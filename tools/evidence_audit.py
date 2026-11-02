#!/usr/bin/env python3
"""Report evidence completeness without making a hardware-support claim."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


RUN_REQUIREMENTS = (
    "cold_shutdown",
    "profile_identity_rechecked",
    "readback_verified",
    "bus_master_disabled",
    "dma_mappings_zero",
    "device_owner_restored",
    "windows_handoff",
    "display_recovered",
)


def audit_evidence(packet: dict[str, Any]) -> dict[str, Any]:
    missing: list[str] = []
    machine = packet.get("machine")
    if not isinstance(machine, dict):
        machine = {}
        missing.append("machine identity")
    for field in ("system_product_name", "bios_version", "gpu_vendor_id", "gpu_device_id", "gpu_subsystem_id", "vbios_version"):
        if not machine.get(field):
            missing.append(f"machine.{field}")

    recovery = packet.get("recovery")
    if not isinstance(recovery, dict):
        recovery = {}
        missing.append("recovery procedure")
    if not isinstance(recovery.get("method"), str) or not recovery["method"].strip():
        missing.append("recovery.method")
    for field in ("independent_of_experimental_efi", "tested_on_exact_machine", "normal_boot_after_test"):
        if recovery.get(field) is not True:
            missing.append(f"recovery.{field}")

    runs = packet.get("cold_boot_runs")
    if not isinstance(runs, list) or len(runs) < 3:
        missing.append("at least three cold_boot_runs")
        runs = runs if isinstance(runs, list) else []
    run_ids: set[str] = set()
    for index, run in enumerate(runs, start=1):
        if not isinstance(run, dict):
            missing.append(f"cold_boot_runs[{index}] is not an object")
            continue
        run_id = run.get("run_id")
        if not isinstance(run_id, str) or not run_id:
            missing.append(f"cold_boot_runs[{index}].run_id")
        elif run_id in run_ids:
            missing.append(f"cold_boot_runs[{index}].run_id is duplicated")
        else:
            run_ids.add(run_id)
        for field in RUN_REQUIREMENTS:
            if run.get(field) is not True:
                missing.append(f"cold_boot_runs[{index}].{field}")

    benchmark = packet.get("benchmark")
    if not isinstance(benchmark, dict):
        benchmark = {}
    for field in ("workload", "precision"):
        if not isinstance(benchmark.get(field), str) or not benchmark[field].strip():
            missing.append(f"benchmark.{field}")
    for field in ("locked_median", "candidate_median"):
        value = benchmark.get(field)
        if not isinstance(value, (int, float)) or isinstance(value, bool) or value <= 0:
            missing.append(f"benchmark.{field} must be a positive number")
    runs_per_state = benchmark.get("runs_per_state")
    if not isinstance(runs_per_state, int) or isinstance(runs_per_state, bool) or runs_per_state < 3:
        missing.append("benchmark.runs_per_state must be at least 3")

    complete = not missing
    return {
        "schema_version": 1,
        "evidence_completeness": "complete-for-review" if complete else "incomplete",
        "cold_boot_run_count": len(runs),
        "distinct_run_ids": len(run_ids),
        "support_claim_allowed": False,
        "human_review_required": True,
        "missing_or_incomplete": missing,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path, help="local evidence JSON packet")
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    try:
        with args.packet.open("r", encoding="utf-8-sig") as stream:
            packet = json.load(stream)
        if not isinstance(packet, dict):
            raise ValueError("evidence packet root must be an object")
        result = audit_evidence(packet)
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            sys.stdout.write(rendered)
    except (OSError, json.JSONDecodeError, TypeError, ValueError) as exc:
        print(f"evidence audit failed: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
