#!/usr/bin/env bash
# Invoke the verilator binary with whatever plusargs the BUILD passed.
# Replaces bzsim's runtime/run.sh orchestration. If the elf path arg is
# non-empty, append `+load=$elf_path` so cosim's whisper sees the program
# file (FLAGS_load in src/cosim/whisper_if/whisper_client.cpp). An empty
# elf path skips the +load arg (used by axi_sw / delay_resp_tb smokes
# which don't preload a program).
set -eu
sim_bin="$1"; shift
elf_path="${1:-}"; [ -n "${1+x}" ] && shift
if [ -n "$elf_path" ]; then
  exec "$sim_bin" "+load=$elf_path" "$@"
else
  exec "$sim_bin" "$@"
fi
