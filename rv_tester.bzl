load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@rv_tester//cosim:cosim.bzl", "cosim_gen")

def rv_tester_gen(name, topology, visibility = None, cc_attrs = {}, **kwargs):

    prefix = "rv_tester"
    rv_tester_dpi = name + "_dpi"
    rv_tester_sv = name + "_sv"

    cosim_gen(
        name = prefix + "_cosim",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    verilog_library(
        name = rv_tester_sv,
        srcs = [
            "@rv_tester//:rv_tester.sv",
            "@rv_tester//:rv_tester_clkgen.sv"
        ],
        deps = [
            "@rv_tester//:defines",
            "@rv_tester//sysmod:sysmod",
            "@rv_tester//transactors/axi_sw:axi_sw",
            topology + "_sv",
        ] + select({
          "@rv_tester//:cosim_off": ["@rv_tester//:no_cosim"],
          "//conditions:default":   [prefix + "_cosim_sv"],
        }),
        visibility = visibility,
    )

    native.cc_library(
        name = rv_tester_dpi,
        srcs = ["@rv_tester//:rv_tester.cpp"],
        deps = [
            "@rv_tester//transactors/axi_sw:axi_sw_dpi",
            "@rv_tester//common:common",
            "@rv_tester//sysmod:sysmod_dpi",
            "@cvm//:plusargs",
            "@cvm//:registry",
            topology + "_cc",
        ] + select({
          "@rv_tester//:cosim_off": [],
          "//conditions:default":   [prefix + "_cosim_dpi"],
        }),
        alwayslink = True,
        visibility = visibility,
    )
