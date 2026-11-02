#!/usr/bin/env python3
"""Audit an offline transaction trace; this module never controls hardware."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


ORDERED_EVENTS = (
    "profile_admitted",
    "preflight_passed",
    "ownership_recorded",
    "mutation_requested",
    "readback_verified",
    "bus_master_disabled",
    "dma_mappings_zero",
    "device_owner_restored",
    "windows_handoff",
)


def audit_trace(trace: dict[str, Any]) -> dict[str, Any]:
    events = trace.get("events")
    errors: list[str] = []
    if not isinstance(events, list) or any(not isinstance(event, str) for event in events):
        events = []
        errors.append("events must be a list of strings")

    if len(events) != len(set(events)):
        errors.append("events must not be duplicated")
    unknown = sorted(set(events) - set(ORDERED_EVENTS) - {"aborted"})
    if unknown:
        errors.append("unknown events: " + ", ".join(unknown))

    positions = [ORDERED_EVENTS.index(event) for event in events if event in ORDERED_EVENTS]
    if positions != sorted(positions):
        errors.append("events are out of order")

    profile_authorized = trace.get("profile_write_authorized") is True
    if "mutation_requested" in events and not profile_authorized:
        errors.append("mutation requested without a reviewed profile authorization")

    cleanup_events = {
        "bus_master_disabled",
        "dma_mappings_zero",
        "device_owner_restored",
    }
    cleanup_complete = cleanup_events.issubset(events)
    if "windows_handoff" in events and not cleanup_complete:
        errors.append("Windows handoff recorded before cleanup invariants")
    if "windows_handoff" in events and "readback_verified" not in events:
        errors.append("Windows handoff recorded before readback verification")

    complete_sequence = all(event in events for event in ORDERED_EVENTS)
    if "aborted" in events:
        state = "aborted"
    elif errors:
        state = "invalid"
    elif complete_sequence and profile_authorized:
        state = "model-complete-needs-human-review"
    else:
        state = "incomplete"

    return {
        "schema_version": 1,
        "state": state,
        "events": events,
        "cleanup_complete": cleanup_complete,
        "hardware_action_performed": False,
        "support_claim_allowed": False,
        "errors": errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path, help="offline JSON event trace")
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    try:
        with args.trace.open("r", encoding="utf-8-sig") as stream:
            trace = json.load(stream)
        if not isinstance(trace, dict):
            raise ValueError("trace root must be an object")
        result = audit_trace(trace)
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            sys.stdout.write(rendered)
    except (OSError, json.JSONDecodeError, TypeError, ValueError) as exc:
        print(f"transaction trace rejected: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
