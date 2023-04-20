# Same as https://aus-gitlab.local.tenstorrent.com/riscv/chips/-/blob/master/compile/compile.sh
# except no additional files (e.g. syscalls) are included.

#! /usr/bin/env bash

set -ex
test=$1
b=$(basename $test | sed 's/\(.*\)\..*/\1/')

/tools_risc/opensrc/riscv-toolchain-12.0.1-newlib/bin/riscv64-unknown-elf-gcc -mcmodel=medany -static -std=gnu99 -O2 -fno-common -fno-builtin-printf -fno-tree-loop-distribute-patterns -nostartfiles -march=rv64ima -mabi=lp64 -lm -lgcc -T $( dirname -- "$0"; )/test.ld $test -o ${b}.elf
/tools_risc/opensrc/riscv-toolchain-12.0.1-newlib/bin/riscv64-unknown-elf-objdump -d ${b}.elf > ${b}.dump
/tools_risc/opensrc/riscv-toolchain-12.0.1-newlib/bin/riscv64-unknown-elf-objcopy -O verilog ${b}.elf ${b}.hex

