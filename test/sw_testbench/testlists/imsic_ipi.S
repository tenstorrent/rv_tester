# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

# Two harts, mhartid 2 and 5, send each other an MSI. Each hart enables
# machine external interrupt id 1 in its own IMSIC file, then stores id 1 to
# the setipnum register of the other hart's file, and spins. The handler
# claims the interrupt through mtopei. The sw_8c DPI stimulus
# retires the same instructions.
.equ IMSIC_MBASE, 0x24000000   # whisper.json
.equ IMSIC_MSTRIDE_LOG2, 12    # mstride 0x1000
.equ HART_A, 2
.equ HART_B, 5

.section .text
.globl _start
.option norvc

_start:
    li    t0, 1
    slli  t0, t0, 31
    addi  t0, t0, 0x100
    csrw  mtvec, t0          # handler, direct mode
    li    t0, 0x70
    csrw  0x350, t0          # miselect = eidelivery
    li    t0, 1
    csrw  0x351, t0          # mireg
    li    t0, 0xc0
    csrw  0x350, t0          # miselect = eie0
    li    t0, 2
    csrw  0x351, t0          # mireg: enable id 1
    li    t0, 0x800
    csrw  mie, t0            # MEIE
    csrsi mstatus, 8         # MIE
    csrr  t1, mhartid
    li    t2, HART_A
    li    t3, HART_B         # the other hart of HART_A
    beq   t1, t2, send
    mv    t3, t2             # the other hart of HART_B
send:
    slli  t3, t3, IMSIC_MSTRIDE_LOG2
    li    t4, IMSIC_MBASE
    add   t4, t4, t3
    li    t0, 1
    sw    t0, 0(t4)          # setipnum_le: interrupt id 1
spin:
    j     spin

.org 0x100
handler:
    csrrw t0, 0x35c, zero    # mtopei: claim the interrupt
done:
    j     done
