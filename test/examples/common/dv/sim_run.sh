#!/usr/bin/env bash
# `bazel run` companion (see rv_tester_sim_run in sim_test.bzl). Declared
# args: <sim.sh> <tb> <memmap.json> <whisper.json> [+plusarg...]; bazel run
# appends the user's ELF (a bare path or +load=<path>) and extra +plusargs.
# Resolves paths against the invocation directory and delegates to sim.sh
# (error check, +dbg, +run_path, ...).
set -uo pipefail

sim_sh="$PWD/$1"; tb="$PWD/$2"; memmap="$PWD/$3"; whisper="$PWD/$4"; shift 4

elf=""
set_elf() {
  if [ -n "$elf" ]; then
    echo "sim_run.sh: more than one ELF given: $elf, $1" >&2
    exit 2
  fi
  elf="$1"
}
args=()
for a in "$@"; do
  case "$a" in
    +load=*) set_elf "${a#+load=}" ;;
    +*) args+=("$a") ;;
    *) set_elf "$a" ;;
  esac
done
if [ -z "$elf" ]; then
  echo "usage: bazel run <target> -- <elf>|+load=<elf> [+plusarg...]" >&2
  exit 2
fi

cd "${BUILD_WORKING_DIRECTORY:-$PWD}"
if [ ! -f "$elf" ]; then
  echo "sim_run.sh: ELF not found: $elf" >&2
  exit 2
fi
elf="$(readlink -f "$elf")"

exec "$sim_sh" "$tb" "+load=$elf" \
    "+memmap_json_path=$memmap" "+whisper_json_path=$whisper" "${args[@]}"
