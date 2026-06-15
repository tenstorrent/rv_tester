#!/usr/bin/env bash
# Trivial driver: invoke the verilator binary with whatever plusargs the
# BUILD passed. Replaces bzsim's runtime/run.sh orchestration. The DPI in
# dpi.cpp feeds a hardcoded instruction (doesn't actually consume the ELF),
# so the elf path arg is ignored here; we just pass the remaining args.
# TODO(open-source): once the testbench actually loads the ELF, wire it
# through a +mem_image=$elf_path plusarg or similar.
set -eu
sim_bin="$1"; shift
elf_path="${1:-}"; [ -n "${1+x}" ] && shift  # ignored — see header
exec "$sim_bin" "$@"
