"""Shared verilator_cc_library `vopts` constants."""

COMMON_VOPTS = [
    "--default-language",
    "1800-2017",
    "-Wall",
    "-Wpedantic",
    "-Wno-UNUSEDSIGNAL",
    "-Wno-PROCASSINIT",
    "-Wno-UNUSEDPARAM",
    "-Wno-GENUNNAMED",
    "-Wno-UNDRIVEN",
    "-Wno-PINMISSING",
    "-Wno-DECLFILENAME",
    "-Wno-PINCONNECTEMPTY",
    "-Wno-WIDTHEXPAND",
    "-Wno-UNUSEDGENVAR",
    "-Wno-SYNCASYNCNET",
    "-Wno-BLKSEQ",
    "-Wno-EOFNEWLINE",
    "--timing",
]

SW_TESTBENCH_VOPTS = COMMON_VOPTS + [
    "+define+DMI_TB_WRITES_UNSUPPORTED",
    "+define+TRACE_CHECKS_UNSUPPORTED",
]
