"""Shared verilator_cc_library `vopts` constants.

`-Wall` and `-Wpedantic` enable verilator's full lint set. Verilator's
default `m_warnFatal = true` (see external/verilator~/src/V3Error.h)
treats any warning that fires as fatal — non-zero exit on the verilator
action — so no `--Werror`-per-rule escalation is needed. `-Wno-<RULE>`
additions below are for warnings the project explicitly silences.
"""

COMMON_VOPTS = [
    "--default-language",
    "1800-2017",
    # Required for the verilator path — top.sv / delay-resp testbench gate
    # their clkgen instantiation on this. Verilator can't simulate the
    # `forever #(...)` oscillator in rv_tester_clkgen.sv without --timing.
    "+define+TB_EXTERNAL_CLOCK",
    "-Wall",
    "-Wpedantic",
    # -Wno-<RULE> waivers below silence warnings the project doesn't (yet)
    # want to fix. Verilator's m_warnFatal=true means each unhandled
    # warning would fail the build. These mostly come from external SV
    # libraries (opensrc-axi / common_cells) and auto-generated SV from
    # rv_tester_gen / topology_gen / axi_sw_gen.
    "-Wno-UNUSEDSIGNAL",  # 139× — autogen + external libs declare signals never read
    "-Wno-PROCASSINIT",   # 83×  — initial-block assigns; autogen pattern
    "-Wno-UNUSEDPARAM",   # 54×  — module parameters unused on this config
    "-Wno-GENUNNAMED",    # 49×  — anonymous generate blocks in axi / common_cells
    "-Wno-UNDRIVEN",      # 44×  — declared-but-undriven; mostly autogen scaffolds
    "-Wno-PINMISSING",    # 25×  — instance with defaulted pins
    "-Wno-DECLFILENAME",  # 15×  — file-vs-module name mismatch
    "-Wno-PINCONNECTEMPTY",  # 6×
    "-Wno-WIDTHEXPAND",   # 2×  — narrow → wide implicit extension
    "-Wno-UNUSEDGENVAR",  # 2×
    "-Wno-SYNCASYNCNET",  # 2× in axi_sw.sv — reset_n used as both sync/async clocked
    "-Wno-BLKSEQ",        # 2×  — blocking assign in sequential block
    "-Wno-EOFNEWLINE",    # 1×
]

SW_TESTBENCH_VOPTS = COMMON_VOPTS + [
    "+define+DMI_TB_WRITES_UNSUPPORTED",
    "+define+TRACE_CHECKS_UNSUPPORTED",
]
