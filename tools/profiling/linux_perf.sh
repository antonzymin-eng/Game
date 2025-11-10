#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<USAGE
Usage: $0 -b <binary> [-o <output_dir>] [-- perf_args]

Captures Linux perf samples for the headless stress harness.
The script wraps \`perf record\` and \`perf report\` so we can
inspect ThreadPool hotspots described in SYSTEM_TEST_005_THREADING.md.

Required arguments:
  -b <binary>         Path to the compiled Mechanica executable.

Optional arguments:
  -o <output_dir>     Directory for perf artifacts (default: perf_runs/\$(date)).
  -a <app_args>       Arguments forwarded to the binary (default: "--stress-test --stress-summary").
  -F <frequency>      Sampling frequency passed to perf record (default: 999).
  -d <duration>       Minimum runtime in seconds before stopping (default: 0, wait for process).
  -h                  Show this help message.

Any arguments after "--" are forwarded directly to perf record.
USAGE
}

BINARY=""
OUTPUT_DIR=""
APP_ARGS="--stress-test --stress-summary"
PERF_FREQ=999
DURATION=0
PERF_EXTRA=()

while getopts ":b:o:a:F:d:h" opt; do
    case "$opt" in
        b)
            BINARY="$OPTARG"
            ;;
        o)
            OUTPUT_DIR="$OPTARG"
            ;;
        a)
            APP_ARGS="$OPTARG"
            ;;
        F)
            PERF_FREQ="$OPTARG"
            ;;
        d)
            DURATION="$OPTARG"
            ;;
        h)
            usage
            exit 0
            ;;
        :)
            echo "Missing value for -$OPTARG" >&2
            usage
            exit 1
            ;;
        \?)
            echo "Unknown option: -$OPTARG" >&2
            usage
            exit 1
            ;;
    esac
done
shift $((OPTIND - 1))

if [[ -z "$BINARY" ]]; then
    echo "Error: binary path is required" >&2
    usage
    exit 1
fi

if [[ ! -x "$BINARY" ]]; then
    echo "Error: $BINARY is not executable" >&2
    exit 1
fi

if [[ -z "$OUTPUT_DIR" ]]; then
    TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
    OUTPUT_DIR="perf_runs/$TIMESTAMP"
fi

mkdir -p "$OUTPUT_DIR"

PERF_RECORD_ARGS=("perf" "record" "-F" "$PERF_FREQ" "-g" "-o" "$OUTPUT_DIR/perf.data")
if [[ $# -gt 0 ]]; then
    PERF_RECORD_ARGS+=("$@")
fi

PERF_STAT_ARGS=("perf" "stat" "-o" "$OUTPUT_DIR/perf_stat.txt")
RUN_COMMAND=("$BINARY")
# shellcheck disable=SC2206
APP_ARGS_ARR=($APP_ARGS)
RUN_COMMAND+=("${APP_ARGS_ARR[@]}")

printf 'Running perf stat...\n'
"${PERF_STAT_ARGS[@]}" -- "${RUN_COMMAND[@]}"

printf 'Running perf record with command: %s\n' "${RUN_COMMAND[*]}"
"${PERF_RECORD_ARGS[@]}" -- "${RUN_COMMAND[@]}" >/dev/null

if [[ "$DURATION" -gt 0 ]]; then
    echo "Sleeping for $DURATION seconds before stopping perf..."
    sleep "$DURATION"
fi

printf 'Generating perf report...\n'
perf report -i "$OUTPUT_DIR/perf.data" --stdio > "$OUTPUT_DIR/perf_report.txt"

if command -v perf > /dev/null && perf list | grep -q call-graph; then
    printf 'Exporting perf script for flame graphs...\n'
    perf script -i "$OUTPUT_DIR/perf.data" > "$OUTPUT_DIR/perf_script.txt"
fi

echo "Perf capture complete. Artifacts saved to $OUTPUT_DIR"
