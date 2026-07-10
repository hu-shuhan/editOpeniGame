#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: run_smoke_test.sh <executable> [timeout_seconds] [-- extra args...]"
  exit 2
fi

EXE="$1"
shift

TIMEOUT_SEC=30
if [ "$#" -gt 0 ] && [[ "$1" =~ ^[0-9]+$ ]]; then
  TIMEOUT_SEC="$1"
  shift
fi

if [ "${1:-}" = "--" ]; then
  shift
fi

if [ ! -x "$EXE" ] && [ ! -f "$EXE" ]; then
  echo "Executable not found: $EXE"
  exit 1
fi

set +e
timeout "${TIMEOUT_SEC}s" "$EXE" "$@"
EC=$?
set -e

if [ "$EC" -eq 0 ] || [ "$EC" -eq 124 ]; then
  exit 0
fi

exit "$EC"
