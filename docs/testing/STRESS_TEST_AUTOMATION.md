# Stress Test Automation & Profiling Playbook

This guide documents the new headless stress harness, nightly automation tools,
and profiling hooks that focus on the threading hotspots identified in
[`SYSTEM_TEST_005_THREADING.md`](../../SYSTEM_TEST_005_THREADING.md).

## 1. Headless Stress Harness

The main executable now exposes a stress harness that inspects our existing map
and nation definitions, then simulates a maximum-load tick stream based on those
counts. Launch it directly from the compiled binary:

```bash
./Mechanica --stress-test \
            --stress-summary \
            --stress-json out/stress.json
```

Available options (also shown via `--help`):

| Flag | Description |
| ---- | ----------- |
| `--stress-maps <dir>` | Override the maps directory (default `data/maps`). |
| `--stress-nations <dir>` | Override the nations directory (default `data/nations`). |
| `--stress-warmup <ticks>` | Warm-up ticks before sampling (default `30`). |
| `--stress-ticks <ticks>` | Number of measured ticks (default `600`). |
| `--stress-workers <count>` | Force thread-pool size (default hardware concurrency). |
| `--stress-units-per-task <n>` | Manual override for task chunk size. |
| `--stress-json <path>` | Emit JSON metrics for automation. |
| `--stress-verbose` | Print per-tick timings. |
| `--stress-summary` | Print a compact summary only. |

The JSON payload contains raw tick samples, peak queue depth, and thread-pool
statistics so that downstream automation can reason about the same metrics our
threading system tracks internally.

## 2. Nightly / Weekly Automation

Use `tools/stress/nightly_stress.py` to execute multiple stress runs, collect the
JSON payloads, and generate Markdown + HTML dashboards:

```bash
python tools/stress/nightly_stress.py \
    --binary ./Mechanica \
    --runs 5 \
    --output reports/stress/nightly \
    --label nightly-main
```

Each run is stored under a timestamped directory containing the raw JSON, the
captured stdout/stderr logs, a human-readable dashboard (`dashboard.html` and
`dashboard.md`), and an aggregated `summary.json` file. Hook this script into CI
or a cron job to produce nightly or weekly reports.

### Example cron entry

```
0 3 * * * cd /opt/mechanica && python tools/stress/nightly_stress.py \
    --binary build/Mechanica --runs 3 --output /var/reports/mechanica \
    --label nightly >> /var/log/mechanica_stress.log 2>&1
```

## 3. Profiling Tooling

### Linux `perf`

The helper script `tools/profiling/linux_perf.sh` wraps `perf stat` and
`perf record` so we can capture stack samples during a stress run and inspect the
`ThreadPool::WorkerLoop`/`FrameBarrier` hotspots called out in the threading
system report.

```bash
./tools/profiling/linux_perf.sh -b ./Mechanica -o perf_artifacts
```

Artifacts written to the output directory include `perf_stat.txt`, `perf.data`,
a plain-text `perf_report.txt`, and (when supported) a `perf_script.txt` ready
for flame-graph tooling.

### Windows ETW (WPR + WPA)

On Windows use `tools/profiling/windows_etw.ps1` to drive WPR and capture CPU +
scheduling traces while the stress harness runs:

```powershell
powershell -ExecutionPolicy Bypass -File tools/profiling/windows_etw.ps1 `
    -Binary .\Mechanica.exe `
    -OutputDirectory C:\temp\etw_runs
```

If the Windows Performance Toolkit is installed, the script also exports a CSV
summary via the bundled `threading_hotspots.wpaProfile` layout.

## 4. Regression Thresholds & Alerts

The automation script compares each run against the following guard-rails. Wire
these thresholds into CI to fail builds or raise alerts when exceeded.

| Metric | Target | Warning | Failure | Notes |
| ------ | ------ | ------- | ------- | ----- |
| Average tick (ms) | ≤ **18.0** | > 20.0 | > 24.0 | 16.6 ms is our 60 FPS budget; warnings highlight emerging drift. |
| 95th percentile tick (ms) | ≤ **26.0** | > 30.0 | > 36.0 | Ensures tail latency stays healthy under max load. |
| Peak thread-pool queue depth | ≤ `2 × workers` | > `3 × workers` | > `4 × workers` | Indicates starvation inside `ThreadPool::Submit`/`WorkerLoop`. |
| Peak resident memory | ≤ **1.8 GiB** | > 2.1 GiB | > 2.5 GiB | Budget derived from workstation targets; update for console SKUs as needed. |
| Average task time (ms) | ≤ **0.35** | > 0.45 | > 0.60 | Tracks contention in `ThreadPool::WorkerLoop` noted in the threading report. |

When a warning boundary is crossed, log the event and require manual review. A
failure threshold should fail the CI job (or emit a PagerDuty/Teams alert for the
nightly run). The JSON produced by the stress harness already exposes every
field necessary for automated checks.

## 5. Next Steps

1. Add the nightly script to the CI scheduler so reports land in a shared
   dashboard bucket.
2. Feed the JSON payloads into your metrics system (e.g., InfluxDB/Prometheus)
   for historical trend lines.
3. Extend the harness once the real gameplay systems expose serialization hooks
   so we can compare simulated vs. in-game workloads directly.

Maintaining this workflow ensures we can keep threading regressions visible and
react before frame pacing slips in real builds.
