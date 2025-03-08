load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def snoop_gen_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    snoop_gen_dpi = name + "_dpi"
    snoop_gen_sv = name + "_sv"

    native.cc_library(
        name = snoop_gen_dpi,
        srcs = [
            "@rv_tester//snoop_gen:snoop_gen.cpp",
            "@rv_tester//snoop_gen:snoop_gen_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//transactors/axi_sw:safe_queue.h",
            "@rv_tester//transactors/axi_sw:axi.h",
            "@rv_tester//sysmod:device.h",
            "@rv_tester//snoop_gen:snoop_gen.hpp",
            "@rv_tester//snoop_gen:snoop_gen_sequence.hpp",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//transactors/axi_sw:axi_sw_mst",
            "@rv_tester//common:transactor",
            "@rv_tester//:structs",
            packet + "_cc",
            "@rv_tester//common:common",
            "@rv_tester//sysmod:device",
            "@mem_manager//:mem_manager",
            "@rv_tester//:plusargs",
            "@cvm//:plusargs",
            "@cvm//:random",
            "@cvm//:logger",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = snoop_gen_sv,
        srcs = ["@rv_tester//snoop_gen:snoop_gen.sv"],
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
