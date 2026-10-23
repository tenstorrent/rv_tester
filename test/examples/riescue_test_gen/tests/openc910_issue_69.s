# SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

;#test.name       openc910_issue_69
;#test.author     abduv@tenstorrent.com
;#test.arch       rv64
;#test.priv       machine
;#test.env        bare_metal
;#test.cpus       1
;#test.paging     disable
;#test.category   arch
;#test.class      riescue_d
;#test.features   ext_fp.disable ext_v.disable
;#test.tags       counters
;#test.summary
;#test.summary    Targets openc910 issue #69: the priv spec says an explicit
;#test.summary    write to minstret suppresses the writer's own retirement
;#test.summary    increment, but C910 applies the registered increment
;#test.summary    (cnt_adder_ff in ct_hpcp_cnt.v) one cycle after the write.
;#test.summary    csrw minstret,zero; nop; csrr minstret reads 2 instead of 1.
;#test.summary    (nop instead of the issue's csrr mcycle so no other CSR
;#test.summary    value enters the lockstep compare.)
;#test.summary

.section .code, "ax"

test_setup:
    ;#test_passed()

# test01: explicit minstret write must suppress the writer's own increment
;#discrete_test(test=test01)
test01:
    csrw minstret, zero
    nop
    csrr t0, minstret
    li   t1, 1
    bne  t0, t1, test01_fail
    ;#test_passed()
test01_fail:
    ;#test_failed()

test_cleanup:
    ;#test_passed()
