#!/usr/bin/env bash
# Wrapper that runs a Verilator-based sw testbench binary and applies
# the stdout error-pattern check that bzsim's runtime/simtest.py
# (SimTest.sim(), lines 488-511) used to apply. Shared by every sh_test
# in test/.../testlists/BUILD.bazel.
#
# First arg is the sim binary; rest are plusargs forwarded to it.
set -u
set -o pipefail

LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT

# Tee through to stdout (bazel captures it) AND to a file we rescan
# after the binary exits.
"$@" 2>&1 | tee "$LOG"
rc=${PIPESTATUS[0]}

if [ "$rc" -ne 0 ]; then
    echo "sim.sh: simulator exited with code $rc" >&2
    exit "$rc"
fi

# cvm::log(cvm::ERROR, ...) prints "Error: ..."; DPI direct prints use
# "ERROR:"; Verilator $fatal prints "Fatal" / "FATAL". `grep -n` so
# failures include the line number in the captured log.
matches=$(grep -nE '\bError\b|ERROR:|\bFatal\b|FATAL' "$LOG" || true)
if [ -n "$matches" ]; then
    echo "sim.sh: detected error pattern(s) in simulator output:" >&2
    echo "$matches" >&2
    exit 1
fi
