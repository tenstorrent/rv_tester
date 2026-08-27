load("@rules_verilator//verilator:defs.bzl", _verilator_cc_library = "verilator_cc_library")

_default_vopts = [
    "--default-language",
    "1800-2017",
    "+define+TB_EXTERNAL_CLOCK",
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
]

def verilator_cc_library(vopts = [], timing = True, *args, **kwargs):
    _verilator_cc_library(vopts = _default_vopts + vopts, timing = timing, *args, **kwargs)
