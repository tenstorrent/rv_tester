# SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

;#test.name       riescue_smoke
;#test.author     abduv@tenstorrent.com
;#test.arch       rv64
;#test.priv       machine
;#test.env        bare_metal
;#test.cpus       1
;#test.paging     disable
;#test.category   arch
;#test.class      riescue_d
;#test.features   ext_fp.disable ext_v.disable
;#test.tags       smoke
;#test.summary
;#test.summary    Smoke test: integer ALU, M-extension mul/div on randomized
;#test.summary    data, memory accesses. M-mode, paging off, RV64IMAC only,
;#test.summary    so it runs on both example cores under rv_tester lockstep.
;#test.summary

;#random_data(name=data1, type=bits32, and_mask=0xfffffff0)
;#random_data(name=data2, type=bits64)

.section .code, "ax"

test_setup:
    ;#test_passed()

# test01: ALU + branch sanity
;#discrete_test(test=test01)
test01:
    li   t0, 0x1234
    li   t1, 0x4321
    add  t2, t0, t1
    li   t3, 0x5555
    bne  t2, t3, test01_fail
    xor  t2, t0, t1
    li   t3, 0x5115
    bne  t2, t3, test01_fail
    slli t2, t0, 4
    srli t2, t2, 4
    bne  t2, t0, test01_fail
    ;#test_passed()
test01_fail:
    ;#test_failed()

# test02: M-extension mul/div against additive reference on random data
;#discrete_test(test=test02)
test02:
    li   t0, data1
    li   t1, 3
    mul  t2, t0, t1
    add  t3, t0, t0
    add  t3, t3, t0
    bne  t2, t3, test02_fail
    divu t4, t2, t1
    bne  t4, t0, test02_fail
    remu t5, t2, t1
    bnez t5, test02_fail
    ;#test_passed()
test02_fail:
    ;#test_failed()

# test03: store/load roundtrip in .data
;#discrete_test(test=test03)
test03:
    la   t0, scratch_data
    li   t1, data2
    sd   t1, 0(t0)
    ld   t2, 0(t0)
    bne  t2, t1, test03_fail
    sw   t1, 8(t0)
    lwu  t3, 8(t0)
    slli t4, t1, 32
    srli t4, t4, 32
    bne  t3, t4, test03_fail
    lbu  t5, 0(t0)
    andi t6, t1, 0xff
    bne  t5, t6, test03_fail
    ;#test_passed()
test03_fail:
    ;#test_failed()

test_cleanup:
    ;#test_passed()

.section .data
scratch_data:
    .dword 0x0
    .dword 0x0
