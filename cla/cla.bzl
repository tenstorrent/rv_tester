load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def cla_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    cla_dpi = name + "_dpi"
    cla_sv = name + "_sv"

    native.cc_library(
        name = cla_dpi,
        srcs = [
            "@rv_tester//cla:cla.cpp",
            "@rv_tester//cla:cla_cfg_seq.cpp",
        ],
        hdrs = [
            "@rv_tester//cla:cla.hpp",
            "@rv_tester//cla:cla_cfg_seq.hpp",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//transactors/axi_sw:axi_sw_mst",
            "@rv_tester//common:transactor",
            "@rv_tester//common:common",
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
        name = cla_sv,
        srcs = ["@rv_tester//cla:cla.sv"],
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
