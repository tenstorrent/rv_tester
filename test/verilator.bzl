load("@rules_verilator//verilator:defs.bzl", _verilator_cc_library = "verilator_cc_library")

_default_vopts = [
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
]

# disable warnings on verilator generated code
_default_copts = ["-w"]

def verilator_cc_library(vopts = [], copts = [], timing = True, *args, **kwargs):
    _verilator_cc_library(vopts = _default_vopts + vopts, copts = _default_copts + copts, timing = timing, *args, **kwargs)
