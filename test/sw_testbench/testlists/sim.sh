#!/usr/bin/env bash
# Wrapper that runs a Verilator-based sw testbench binary and applies
# the stdout error-pattern check that bzsim's runtime/simtest.py
# (SimTest.sim(), lines 488-511) used to apply. Shared by every sh_test
# in test/.../testlists/BUILD.bazel.
#
# First arg is the sim binary; rest are plusargs forwarded to it.
#
# SIM_WRAP (optional, e.g. via bazel --test_env) is a command prepended to
# the sim binary invocation, such as "valgrind --tool=memcheck ...". Only the
# sim binary is wrapped: the helper tools this script and the sim itself
# spawn (mktemp, tee, grep, objcopy, nm) report their own valgrind findings
# and would otherwise fail the test.
set -u
set -o pipefail

LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT

read -ra sim_wrap <<< "${SIM_WRAP:-}"

# Tee through to stdout (bazel captures it) AND to a file we rescan
# after the binary exits.
"${sim_wrap[@]}" "$@" 2>&1 | tee "$LOG"
rc=${PIPESTATUS[0]}

# EXPECT_ERROR (regex): pass only if the sim reports it after EXPECT_AFTER (regex), with no crash or sanitizer report.
if [ -n "${EXPECT_ERROR:-}" ]; then
    if [ "$rc" -ge 128 ] || grep -qE 'Sanitizer|runtime error:' "$LOG"; then
        echo "sim.sh: crash or sanitizer report (rc=$rc)" >&2
        exit 1
    fi
    after=$(grep -nE -m1 "${EXPECT_AFTER:-.}" "$LOG" | cut -d: -f1)
    err=$(grep -nE -m1 "$EXPECT_ERROR" "$LOG" | cut -d: -f1)
    [ -n "$after" ] && [ -n "$err" ] && [ "$err" -gt "$after" ] && exit 0
    echo "sim.sh: expected '$EXPECT_ERROR' after '${EXPECT_AFTER:-.}'" >&2
    exit 1
fi

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
