load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")
load("@rv_tester//cosim:cosim.bzl", "cosim_gen")
load("@rv_tester//sysmod:sysmod.bzl", "sysmod_gen")
load("@rv_tester//transactors/axi_sw:axi_sw.bzl", "axi_sw_gen")

def rv_tester_gen(name, topology, visibility = None, cc_attrs = {}, **kwargs):

    rv_tester_dpi = name + "_dpi"
    rv_tester_assert_dpi = name + "_assert_dpi"
    rv_tester_sv = name + "_sv"

    verilog_library(
        name = name + "_harness",
        srcs = [
            "@rv_tester//:rv_tester_pkg.sv",
            "@rv_tester//:rv_tester_defines.sv",
        ],
        deps = [
            topology + "_sv",
        ],
        visibility = visibility,
    )

    packet_gen(
        name = name + "_transactions",
        src = "@rv_tester//:rv_tester_transactions.yml",
        package = "rv_tester_transactions",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    cosim_gen(
        name = name + "_cosim",
        packet = name + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    sysmod_gen(
        name = name + "_sysmod",
        packet = name + "_transactions",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    axi_sw_gen(
        name = name + "_axi_sw",
        packet = name + "_transactions",
        cc_attrs = cc_attrs,
    )

    verilog_library(
        name = rv_tester_sv,
        srcs = [
            "@rv_tester//:rv_tester.sv",
            "@rv_tester//:rv_tester_clkgen.sv"
        ],
        deps = [
            name + "_harness",
            name + "_transactions_sv",
            name + "_sysmod_sv",
            name + "_axi_sw_sv",
        ] + select({
          "@rv_tester//:cosim_off": ["@rv_tester//:no_cosim"],
          "//conditions:default":   [name + "_cosim_sv"],
        }),
        visibility = visibility,
    )

    native.cc_library(
        name = rv_tester_dpi,
        srcs = ["@rv_tester//:rv_tester.cpp"],
        deps = [
            "@rv_tester//common:common",
            "@cvm//:plusargs",
            "@cvm//:registry",
            name + "_transactions_cc",
            name + "_sysmod_dpi",
            name + "_axi_sw_dpi",
            "@rv_tester//pmcounters:pmcounters",
            topology + "_cc",
        ] + select({
          "@rv_tester//:cosim_off": [],
          "//conditions:default":   [name + "_cosim_dpi"],
        }),
        alwayslink = True,
        visibility = visibility,
    )

    native.cc_library(
        name = rv_tester_assert_dpi,
        srcs = ["@rv_tester//:rv_tester_assert_handler.cpp"],
        deps = ["@cvm//:logger"],
        alwayslink = True,
        visibility = visibility,
    )
