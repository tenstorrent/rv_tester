load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def interrupts_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    interrupts_dpi = name + "_dpi"
    interrupts_sv = name + "_sv"

    native.cc_library(
        name = interrupts_dpi,
        srcs = [
            "@rv_tester//interrupts:interrupts.cpp",
            "@rv_tester//interrupts:nmi_sequence.cpp",
            "@rv_tester//interrupts:external_interrupt_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//interrupts:interrupts.hpp",
            "@rv_tester//interrupts:nmi_sequence.hpp",
            "@rv_tester//interrupts:external_interrupt_sequence.hpp",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//:plusargs",
            "@rv_tester//common:transactor",
            "@rv_tester//:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:random",
            "@cvm//:logger",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = interrupts_sv,
        srcs = ["@rv_tester//interrupts:interrupts.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
