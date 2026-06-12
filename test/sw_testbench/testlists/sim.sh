#!/usr/bin/env bash
# Trivial driver: invoke the verilator binary with the elf path + plusargs.
# Replaces bzsim's runtime/run.sh orchestration; the testbench DPI in
# dpi.cpp doesn't actually load the elf today (it feeds a hardcoded
# instruction), so passing the elf is forward-compat for when that lands.
set -euo pipefail
sim_bin="$1"; shift
elf_path="$1"; shift
exec "$sim_bin" "+elf=$elf_path" "$@"
