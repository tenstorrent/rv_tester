load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")
load("@rv_tester//cosim:cosim.bzl", "cosim_gen")
load("@rv_tester//sysmod:sysmod.bzl", "sysmod_gen")
load("@rv_tester//pmu:pmu.bzl", "pmu_gen")
load("@rv_tester//dm_model:dm_model.bzl", "dm_model_gen")
load("@rv_tester//aplic_monitor:aplic_monitor.bzl", "aplic_monitor_gen")
load("@rv_tester//pwrmgmt:pwrmgmt.bzl", "pwrmgmt_gen")
load("@rv_tester//aclint_checker:aclint_checker.bzl", "aclint_checker_gen")
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
            "@rv_tester//:dmi_driver.sv",
            "@rv_tester//:rv_tester_stall_checker.sv",
        ],
        deps = [
            topology + "_sv",
	    "@opensrc-axi//:axi",
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

    pmu_gen(
        name = name + "_pmu",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    dm_model_gen(
        name = name + "_dm_model",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )
    
    aplic_monitor_gen(
        name = name + "_aplic_monitor",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    pwrmgmt_gen(
        name = name + "_pwrmgmt",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    aclint_checker_gen(
        name = name + "_aclint_checker",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
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
            "@rv_tester//:rv_tester_clkgen.sv",
            "@rv_tester//:rv_tester_mem.sv",
        ],
        deps = [
            "@cvm//:logger_sv",
            name + "_harness",
            name + "_transactions_sv",
            name + "_sysmod_sv",
            name + "_pmu_sv",
            name + "_dm_model_sv",
            name + "_aplic_monitor_sv",
            name + "_pwrmgmt_sv",
            name + "_aclint_checker_sv",
            name + "_axi_sw_sv",
            "@opensrc-axi_llc//:axi_llc",
            "@opensrc-axi//:axi",
            "@opensrc-tech_cells_generic//:tech_cells_generic"
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
            "@rv_tester//:plusargs",
            "@rv_tester//:structs",
            "@rv_tester//common:common",
            "@cvm//:plusargs",
            "@cvm//:plusargs_dpi",
            "@cvm//:registry",
            "@aplic//:Aplic",
            name + "_transactions_cc",
            name + "_sysmod_dpi",
            name + "_pmu_dpi",
            name + "_dm_model_dpi",
            name + "_aplic_monitor_dpi",
            name + "_pwrmgmt_dpi",
            name + "_aclint_checker_dpi",
            name + "_axi_sw_dpi",
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
        deps = ["@cvm//:logger", "@cvm//:plusargs", "@cvm//:plusargs_dpi"],
        alwayslink = True,
        visibility = visibility,
    )
