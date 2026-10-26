"""Verilator options shared by every example, on top of the repo defaults that
//test:verilator.bzl applies (lint flags, --timing). Each example's
dv/verilator_opts.bzl extends SW_TESTBENCH_VOPTS with its own core-specific
waivers."""

SW_TESTBENCH_VOPTS = [
    "+define+DMI_TB_WRITES_UNSUPPORTED",
    "+define+TRACE_CHECKS_UNSUPPORTED",
]
