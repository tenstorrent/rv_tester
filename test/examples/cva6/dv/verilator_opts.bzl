load("//test/examples/common/dv:verilator_opts.bzl", "SW_TESTBENCH_VOPTS")

# CVA6: enable RVFI, disable PMU (not present), waive stock lint warnings.
CVA6_VOPTS = SW_TESTBENCH_VOPTS + [
    "+define+RV_TESTER_PMCI_DISABLE",
    "+define+RVFI_TRACE",
    "+define+RVFI_MEM",
    "-Wno-ALWCOMBORDER",
    "-Wno-BLKANDNBLK",
    "-Wno-CASEINCOMPLETE",
    "-Wno-CMPCONST",
    "-Wno-IMPLICITSTATIC",
    "-Wno-IMPORTSTAR",
    "-Wno-LATCH",
    "-Wno-LITENDIAN",
    "-Wno-SELRANGE",
    "-Wno-SYMRSVDWORD",
    "-Wno-UNOPTFLAT",
    "-Wno-UNPACKED",
    "-Wno-UNSIGNED",
    "-Wno-UNUSED",
    "-Wno-VARHIDDEN",
    "-Wno-WIDTH",
    "-Wno-WIDTHCONCAT",
    "-Wno-WIDTHTRUNC",
    "-Wno-CASEOVERLAP",
    "-Wno-ASCRANGE",
]
