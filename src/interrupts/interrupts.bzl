load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def interrupts_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    interrupts_dpi = name + "_dpi"
    interrupts_sv = name + "_sv"

    native.cc_library(
        name = interrupts_dpi,
        srcs = [
            "@rv_tester//src/interrupts:interrupts.cpp",
            "@rv_tester//src/interrupts:nmi_sequence.cpp",
            "@rv_tester//src/interrupts:external_interrupt_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//src/interrupts:interrupts.hpp",
            "@rv_tester//src/interrupts:nmi_sequence.hpp",
            "@rv_tester//src/interrupts:external_interrupt_sequence.hpp",
        ],
        deps = [
            "@rv_tester//src/sysmod:sysmod_plusargs",
            "@rv_tester//src/sysmod/trickbox:interrupter_cc",
            "@rv_tester//src:plusargs",
            "@rv_tester//src/cosim/whisper_if:whisper_if",
            "@rv_tester//src/cosim/bridge:bridge_plusargs",
            "@rv_tester//src/common:transactor",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst",
            "@rv_tester//src:structs",
            "@rv_tester//src:device_handler",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:random",
            "@cvm//:logger",
            "@cvm//:registry",
            "@cvm//:topology",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = interrupts_sv,
        srcs = ["@rv_tester//src/interrupts:interrupts.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            "@cvm//:registry_sv",
            "@rv_tester//src:rv_tester_tick_generator",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
