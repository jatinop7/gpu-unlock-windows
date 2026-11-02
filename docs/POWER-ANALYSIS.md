# Power and operating-state analysis

## Reference run telemetry

The reference benchmark record contains throughput results but no synchronized GPU telemetry. The transaction and benchmark outputs provided for this run do not include power limits, power draw, clocks, voltage, temperature, or clock-event/throttle samples.

| Metric | Baseline run | Candidate run |
|---|---|---|
| GPU power limit / enforced limit | Not present in the run record | Not present in the run record |
| GPU power draw | Not present in the run record | Not present in the run record |
| Graphics and memory clocks | Not present in the run record | Not present in the run record |
| Voltage | Not present in the run record | Not present in the run record |
| GPU temperature | Not present in the run record | Not present in the run record |
| Clock event / throttle reasons | Not present in the run record | Not present in the run record |

The [system inventory](../artifacts/system-inventory.json) records the machine and driver information collected while preparing this evidence packet. It is not a power sample captured during the original GEMM runs.

## Capture power and throttling data

NVIDIA documents power draw and limits, current clocks, temperatures, and clock-event reasons through `nvidia-smi`; support varies by GPU and driver. Run a one-second telemetry capture alongside each benchmark and preserve separate baseline and candidate files:

```powershell
nvidia-smi -q -d POWER,CLOCK,TEMPERATURE,PERFORMANCE -l 1 -f .\artifacts\power-baseline.txt
```

Stop the capture with `Ctrl+C` after the baseline benchmark finishes. Repeat with `power-candidate.txt` while running the candidate benchmark. Compare GPU power draw with the requested/enforced power limit, graphics and memory clocks, temperature, and active clock-event reasons. `nvidia-smi -q` also reports whether voltage information is available; current NVIDIA documentation notes that graphics voltage reporting has been deprecated and may be `N/A`.

NVIDIA describes power management as operating within a power envelope and identifies power and thermal limits as factors that can reduce GPU clocks. Those signals are needed to distinguish a power-state change from a thermal, clock, or workload difference in the RTX 4050 result. See the [NVIDIA `nvidia-smi` reference](https://docs.nvidia.com/deploy/nvidia-smi/).
