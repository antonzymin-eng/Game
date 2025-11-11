#!/usr/bin/env python3
"""Run the stress harness on a schedule and build lightweight dashboards."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import shlex
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Execute the Mechanica stress harness and aggregate metrics."
    )
    parser.add_argument("--binary", required=True, help="Path to the compiled Mechanica executable")
    parser.add_argument("--runs", type=int, default=3, help="Number of runs per invocation (default: 3)")
    parser.add_argument(
        "--output",
        default="stress_reports",
        help="Directory where dashboards and JSON summaries are written",
    )
    parser.add_argument(
        "--stress-args",
        default="",
        help="Additional arguments passed to the stress harness (default: none)",
    )
    parser.add_argument(
        "--label",
        default="nightly",
        help="Optional label included in the dashboard metadata",
    )
    return parser.parse_args()


def ensure_executable(path: Path) -> None:
    if not path.exists():
        raise FileNotFoundError(f"Binary '{path}' does not exist")
    if not os.access(path, os.X_OK):
        raise PermissionError(f"Binary '{path}' is not executable")


def run_stress_once(binary: Path, run_index: int, run_dir: Path, extra_args: str) -> Dict[str, Any]:
    run_output_dir = run_dir / f"run_{run_index:02d}"
    run_output_dir.mkdir(parents=True, exist_ok=True)
    json_path = run_output_dir / "stress_metrics.json"

    base_command = [str(binary), "--stress-test", "--stress-json", str(json_path)]
    if extra_args:
        base_command.extend(shlex.split(extra_args))

    print(f"[stress-automation] Running: {' '.join(base_command)}")
    completed = subprocess.run(base_command, capture_output=True, text=True)
    if completed.returncode != 0:
        sys.stderr.write(completed.stdout)
        sys.stderr.write(completed.stderr)
        raise RuntimeError(f"Stress harness exited with code {completed.returncode}")

    if not json_path.exists():
        raise RuntimeError(f"Stress harness did not produce metrics file: {json_path}")

    with json_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    # Write captured stdout/stderr for traceability
    (run_output_dir / "stdout.log").write_text(completed.stdout, encoding="utf-8")
    (run_output_dir / "stderr.log").write_text(completed.stderr, encoding="utf-8")

    return data


def aggregate_runs(results: List[Dict[str, Any]]) -> Dict[str, Any]:
    tick_avgs = [entry["metrics"]["average_tick_ms"] for entry in results]
    p95s = [entry["metrics"].get("p95_tick_ms", 0.0) for entry in results]
    mem_usage = [entry["metrics"].get("resident_memory_kb", 0.0) for entry in results]

    aggregate: Dict[str, Any] = {
        "mean_average_tick_ms": statistics.mean(tick_avgs) if tick_avgs else 0.0,
        "median_average_tick_ms": statistics.median(tick_avgs) if tick_avgs else 0.0,
        "mean_p95_tick_ms": statistics.mean(p95s) if p95s else 0.0,
        "peak_p95_tick_ms": max(p95s) if p95s else 0.0,
        "peak_memory_kb": max(mem_usage) if mem_usage else 0.0,
        "mean_memory_kb": statistics.mean(mem_usage) if mem_usage else 0.0,
        "per_run": [],
    }

    for entry in results:
        metrics = entry["metrics"]
        aggregate["per_run"].append(
            {
                "timestamp": metrics.get("timestamp_utc"),
                "average_tick_ms": metrics.get("average_tick_ms"),
                "p95_tick_ms": metrics.get("p95_tick_ms"),
                "max_tick_ms": metrics.get("max_tick_ms"),
                "resident_memory_kb": metrics.get("resident_memory_kb"),
                "peak_active_tasks": metrics.get("peak_active_tasks"),
                "peak_queue_depth": metrics.get("peak_queue_depth"),
            }
        )

    return aggregate


def render_dashboard(aggregate: Dict[str, Any], results: List[Dict[str, Any]], output_dir: Path, label: str) -> None:
    generated_at = dt.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    html_path = output_dir / "dashboard.html"
    md_path = output_dir / "dashboard.md"

    rows = []
    for idx, entry in enumerate(aggregate["per_run"], start=1):
        rows.append(
            f"<tr><td>{idx}</td><td>{entry['timestamp']}</td><td>{entry['average_tick_ms']:.2f}</td>"
            f"<td>{entry['p95_tick_ms']:.2f}</td><td>{entry['max_tick_ms']:.2f}</td>"
            f"<td>{entry['resident_memory_kb'] / 1024.0:.1f}</td>"
            f"<td>{entry['peak_active_tasks']}</td><td>{entry['peak_queue_depth']}</td></tr>"
        )

    html = f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
<meta charset=\"utf-8\" />
<title>Mechanica Stress Dashboard</title>
<style>
body {{ font-family: Arial, sans-serif; margin: 2rem; }}
summary {{ font-weight: bold; }}
table {{ border-collapse: collapse; width: 100%; margin-top: 1rem; }}
th, td {{ border: 1px solid #ccc; padding: 0.5rem; text-align: right; }}
th {{ background-color: #f5f5f5; }}
</style>
</head>
<body>
<h1>Stress Automation Report ({label})</h1>
<p>Generated at {generated_at}</p>
<section>
  <summary>Aggregate</summary>
  <ul>
    <li>Mean average tick: {aggregate['mean_average_tick_ms']:.2f} ms</li>
    <li>Median average tick: {aggregate['median_average_tick_ms']:.2f} ms</li>
    <li>Mean p95 tick: {aggregate['mean_p95_tick_ms']:.2f} ms (peak {aggregate['peak_p95_tick_ms']:.2f} ms)</li>
    <li>Mean resident memory: {aggregate['mean_memory_kb'] / 1024.0:.1f} MiB (peak {aggregate['peak_memory_kb'] / 1024.0:.1f} MiB)</li>
  </ul>
</section>
<section>
  <table>
    <thead>
      <tr><th>Run</th><th>Timestamp (UTC)</th><th>Avg Tick (ms)</th><th>P95 Tick (ms)</th><th>Max Tick (ms)</th><th>Resident MiB</th><th>Peak Active</th><th>Peak Queue</th></tr>
    </thead>
    <tbody>
      {''.join(rows)}
    </tbody>
  </table>
</section>
</body>
</html>
"""

    html_path.write_text(html, encoding="utf-8")

    with md_path.open("w", encoding="utf-8") as handle:
        handle.write(f"# Stress Automation Report ({label})\n\n")
        handle.write(f"Generated at **{generated_at}**\n\n")
        handle.write(
            "| Run | Timestamp (UTC) | Avg Tick (ms) | P95 Tick (ms) | Max Tick (ms) | Resident MiB | Peak Active | Peak Queue |\n"
        )
        handle.write("| --- | --- | --- | --- | --- | --- | --- | --- |\n")
        for idx, entry in enumerate(aggregate["per_run"], start=1):
            handle.write(
                f"| {idx} | {entry['timestamp']} | {entry['average_tick_ms']:.2f} | {entry['p95_tick_ms']:.2f}"
                f" | {entry['max_tick_ms']:.2f} | {entry['resident_memory_kb'] / 1024.0:.1f}"
                f" | {entry['peak_active_tasks']} | {entry['peak_queue_depth']} |\n"
            )
        handle.write("\n")
        handle.write(
            f"**Mean avg tick:** {aggregate['mean_average_tick_ms']:.2f} ms · "
            f"**Median avg tick:** {aggregate['median_average_tick_ms']:.2f} ms · "
            f"**Mean p95:** {aggregate['mean_p95_tick_ms']:.2f} ms (peak {aggregate['peak_p95_tick_ms']:.2f} ms) · "
            f"**Mean resident mem:** {aggregate['mean_memory_kb'] / 1024.0:.1f} MiB (peak {aggregate['peak_memory_kb'] / 1024.0:.1f} MiB)\n"
        )

    summary_json = {
        "generated_at": generated_at,
        "label": label,
        "aggregate": aggregate,
        "runs": results,
    }
    (output_dir / "summary.json").write_text(json.dumps(summary_json, indent=2), encoding="utf-8")


def main() -> None:
    args = parse_args()
    binary = Path(args.binary).resolve()
    ensure_executable(binary)

    timestamp = dt.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")
    output_dir = Path(args.output).resolve() / timestamp
    output_dir.mkdir(parents=True, exist_ok=True)

    run_results: List[Dict[str, Any]] = []
    for run_index in range(1, args.runs + 1):
        result = run_stress_once(binary, run_index, output_dir, args.stress_args)
        run_results.append(result)

    aggregate = aggregate_runs(run_results)
    render_dashboard(aggregate, run_results, output_dir, args.label)
    print(f"Dashboard written to {output_dir}")


if __name__ == "__main__":
    main()
