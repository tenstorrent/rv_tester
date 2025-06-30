load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")
load("@rv_tester//cosim:cosim.bzl", "cosim_gen")
load("@rv_tester//sysmod:sysmod.bzl", "sysmod_gen")
load("@rv_tester//pmu:pmu.bzl", "pmu_gen")
load("@rv_tester//dm_model:dm_model.bzl", "dm_model_gen")
load("@rv_tester//pwrmgmt:pwrmgmt.bzl", "pwrmgmt_gen")
load("@rv_tester//interrupts:interrupts.bzl", "interrupts_gen")
load("@rv_tester//jtag_driver:jtag_driver.bzl", "jtag_driver_gen")
load("@rv_tester//overlay_driver:overlay_driver.bzl", "overlay_driver_gen")
load("@rv_tester//snoop_gen:snoop_gen.bzl", "snoop_gen_gen")
load("@rv_tester//trace:trace.bzl", "trace_gen")
load("@rv_tester//cla:cla.bzl", "cla_gen")
load("@rv_tester//triggers:triggers.bzl", "triggers_gen")
load("@rv_tester//aclint_checker:aclint_checker.bzl", "aclint_checker_gen")
load("@rv_tester//transactors/axi_sw:axi_sw.bzl", "axi_sw_gen")
load("@rv_tester//csr:csr_param_gen.bzl", "csr_param_gen")

def rv_tester_gen(name, topology, csr_spec = "@rv_tester//csr:csr_spec", visibility = None, cc_attrs = {}, **kwargs):
    """Generate rv_tester build targets with CSR collateral available to all modules.
    
    This function generates CSR collateral files and makes them available as dependencies:
    - csr_param_hpp: C++ header file for CSR definitions ({name}_csr_param.hpp)
    - {name}_csr_param_sv: SystemVerilog file for CSR definitions ({name}_csr_param.sv)
    
    The CSR collateral is automatically included in:
    - SystemVerilog modules: {name}_csr_param_sv is added to verilog_library deps
    - C++ modules: {name}_csr_param_hpp is added to cc_library deps
    
    
    Args:
        name: Base name for generated targets
        topology: Topology target 
        csr_spec: CSR specification file
        visibility: Target visibility
        cc_attrs: C++ compilation attributes
        **kwargs: Additional arguments
    """

    rv_tester_dpi = name + "_dpi"
    rv_tester_assert_dpi = name + "_assert_dpi"
    rv_tester_sv = name + "_sv"

    csr_param_gen(
        name = name + "_csr_param",
        csr_spec = csr_spec,
        package = "csr_param",
        cc_attrs = cc_attrs,
    )

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
        csr_param = name + "_csr_param",
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

    pwrmgmt_gen(
        name = name + "_pwrmgmt",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    interrupts_gen(
        name = name + "_interrupts",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    jtag_driver_gen(
        name = name + "_jtag_driver",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )       

    overlay_driver_gen(
        name = name + "_overlay_driver",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )       

    snoop_gen_gen(
        name = name + "_snoop_gen",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )

    trace_gen(
        name = name + "_trace",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )        

    cla_gen(
        name = name + "_cla",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        cc_attrs = cc_attrs,
    )      

    triggers_gen(
        name = name + "_triggers",
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
            "@rv_tester//:rv_tester_lib.sv",
        ],
        deps = [
            "@cvm//:logger_sv",
            name + "_harness",
            name + "_transactions_sv",
            name + "_sysmod_sv",
            name + "_pmu_sv",
            name + "_dm_model_sv",
            name + "_aclint_checker_sv",
            name + "_interrupts_sv",
            name + "_jtag_driver_sv",
            name + "_overlay_driver_sv",
            name + "_snoop_gen_sv",
            name + "_trace_sv",
            name + "_cla_sv",
            name + "_triggers_sv",
            name + "_axi_sw_sv",
            "@opensrc-axi_llc//:axi_llc",
            "@opensrc-axi//:axi",
            "@opensrc-tech_cells_generic//:tech_cells_generic"
        ] + select({
          "@rv_tester//:cosim_off": ["@rv_tester//:no_cosim"],
          "//conditions:default":   [name + "_cosim_sv"],
        }) + select({
          "@rv_tester//:pwrmgmt_off": [],
          "//conditions:default":   [name + "_pwrmgmt_sv"],
        }),
        visibility = visibility,
    )

    native.cc_library(
        name = rv_tester_dpi,
        srcs = ["@rv_tester//:rv_tester.cpp"],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//:structs",
            "@rv_tester//common:common",
            "@rv_tester//preload_axi_llc:preload_axi_llc",
            "@cvm//:plusargs",
            "@cvm//:random",
            "@cvm//:registry",
            name + "_transactions_cc",
            name + "_sysmod_dpi",
            name + "_pmu_dpi",
            name + "_dm_model_dpi",
            name + "_aclint_checker_dpi",
            name + "_interrupts_dpi",
            name + "_jtag_driver_dpi",
            name + "_overlay_driver_dpi",
            name + "_snoop_gen_dpi",
            name + "_trace_dpi",
            name + "_cla_dpi",
            name + "_triggers_dpi",
            name + "_axi_sw_dpi",
            topology + "_cc",
        ] + select({
          "@rv_tester//:cosim_off": [],
          "//conditions:default":   [name + "_cosim_dpi"],
        }) + select({
          "@rv_tester//:pwrmgmt_off": [],
          "//conditions:default":   [name + "_pwrmgmt_dpi"],
        }),
        alwayslink = True,
        visibility = visibility,
    )

    native.cc_library(
        name = rv_tester_assert_dpi,
        srcs = ["@rv_tester//:rv_tester_assert_handler.cpp"],
        deps = ["@cvm//:logger", "@cvm//:plusargs"],
        alwayslink = True,
        visibility = visibility,
    )
