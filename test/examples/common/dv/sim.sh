#!/usr/bin/env bash
# Wrapper: runs Verilator testbench, applies stdout error-pattern check, archives on failure.
# Shared by every example; core-specific plusargs belong in the sh_test args, not here.
# Everything printed to the terminal (simulator stdout+stderr and this
# script's own messages) is also written to run.log in the working directory.
set -u
set -o pipefail

# Extract control flags (consumed, not forwarded):
#   +save_all_files     archive on clean pass
#   +dbg[=start:end]    enable VCD dump; optional cycle window
#                       +dbg only produces a dump if the model was verilated trace-capable.
#   +run_path=<dir>     run the simulation in <dir>
save_all=0
run_path=""
args=()
for a in "$@"; do
    case "$a" in
        +save_all_files)
            save_all=1
            ;;
        +run_path=*)
            run_path="${a#+run_path=}"
            ;;
        +dbg)
            args+=("+vcd_cycle_on=0")
            save_all=1
            ;;
        +dbg=*)
            win="${a#+dbg=}"
            args+=("+vcd_cycle_on=${win%%:*}")
            [ "$win" != "${win#*:}" ] && args+=("+vcd_cycle_off=${win#*:}")
            save_all=1
            ;;
        *)
            args+=("$a")
            ;;
    esac
done

if [ -n "$run_path" ]; then
    case "$run_path" in
        /*) ;;
        *) run_path="${BUILD_WORKING_DIRECTORY:-$PWD}/$run_path" ;;
    esac
    if [ -z "${BUILD_WORKING_DIRECTORY:-}" ] && [ -n "${TEST_TARGET:-}" ]; then
        sub="${TEST_TARGET#//}"; sub="${sub#@*//}"; sub="${sub//:/\/}"
        [ -n "${TEST_SHARD_INDEX:-}" ] && sub="$sub/shard_$TEST_SHARD_INDEX"
        run_path="$run_path/$sub"
    fi
    mkdir -p "$run_path" || exit 1
    for i in "${!args[@]}"; do
        a="${args[$i]}"
        case "$a" in
            +load=*|+memmap_json_path=*|+whisper_json_path=*)
                v="${a#*=}"
                if [ "${v#/}" = "$v" ] && [ -e "$v" ]; then
                    args[$i]="${a%%=*}=$PWD/$v"
                fi
                ;;
            *)
                if [ "$i" -eq 0 ] && [ "${a#/}" = "$a" ] && [ -e "$a" ]; then
                    args[$i]="$PWD/$a"
                fi
                ;;
        esac
    done
    cd "$run_path" || exit 1
fi

OUT="${TEST_UNDECLARED_OUTPUTS_DIR:-}"
[ -n "${BUILD_WORKING_DIRECTORY:-}" ] && OUT=""
before=$(ls -1A 2>/dev/null | sort)
marker=$(mktemp)
trap 'rm -f "$marker"' EXIT

LOG="$PWD/run.log"
{
    echo "# $(date '+%Y-%m-%d %H:%M:%S')  cwd: $PWD"
    printf '# cmd:'; printf ' %q' "${args[@]}"; echo
} > "$LOG"
say() { echo "$@" | tee -a "$LOG" >&2; }
[ -n "$run_path" ] && say "sim.sh: running in $run_path"

"${args[@]}" 2>&1 | tee -a "$LOG"
rc=${PIPESTATUS[0]}

# Match cvm "Error:", DPI "ERROR:", Verilator $fatal "Fatal/FATAL"; -n for line numbers.
# Skip the header lines so the +cmd line's own text can't trigger a match.
matches=$(tail -n +3 "$LOG" | grep -nE '\bError\b|ERROR:|\bFatal\b|FATAL' || true)
# A run that never reaches the end-of-test check (no clock, bridge never
# started) exits 0 with an empty log; the pass marker MUST be present.
passed=$(grep -c 'Pass condition detected' "$LOG" || true)
failed=0
[ "$rc" -ne 0 ] && failed=1
[ -n "$matches" ] && failed=1
[ "$passed" -eq 0 ] && failed=1

if [ "$rc" -ne 0 ]; then
    say "sim.sh: simulator exited with code $rc"
fi
if [ -n "$matches" ]; then
    say "sim.sh: detected error pattern(s) in simulator output:"
    say "$matches"
fi
if [ "$passed" -eq 0 ]; then
    say "sim.sh: no 'Pass condition detected' in simulator output"
fi

if [ -n "$OUT" ] && { [ "$failed" -eq 1 ] || [ "$save_all" -eq 1 ]; }; then
    after=$(ls -1A 2>/dev/null | sort)
    {
        comm -13 <(printf '%s\n' "$before") <(printf '%s\n' "$after")
        find . -mindepth 1 -maxdepth 1 -newer "$marker" -printf '%P\n' 2>/dev/null
    } | sort -u | while IFS= read -r f; do
        [ -n "$f" ] && cp -r -- "$f" "$OUT/" 2>/dev/null || true
    done
    cp -- "$LOG" "$OUT/run.log" 2>/dev/null || true
fi

[ "$rc" -ne 0 ] && exit "$rc"
[ "$failed" -ne 0 ] && exit 1
exit 0
