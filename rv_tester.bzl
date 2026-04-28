load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")
load("@rv_tester//cosim:cosim.bzl", "cosim_gen")
load("@rv_tester//sysmod:sysmod.bzl", "sysmod_gen")
load("@rv_tester//pmu:pmu.bzl", "pmu_gen")
load("@rv_tester//pmu:pmu_fragment_gen.bzl", "pmu_fragment_gen")
load("@rv_tester//interrupts:interrupts.bzl", "interrupts_gen")
load("@rv_tester//triggers:triggers.bzl", "triggers_gen")
load("@rv_tester//transactors/axi_sw:axi_sw.bzl", "axi_sw_gen")
load("@rv_tester//csr:csr_param_gen.bzl", "csr_param_gen")

def rv_tester_gen(
    name, 
    topology, 
    csr_spec = "@rv_tester//csr:csr_spec",
    csr_param_override = "@rv_tester//csr:csr_param_override",
    pmu_spec = "@rv_tester//pmu:pmu_spec",
    visibility = None, 
    cc_attrs = {}, 
    **kwargs
):
    """Generate rv_tester build targets with CSR and PMU collateral available to all modules.
    
    This function generates CSR and PMU collateral files and makes them available as dependencies:
    - csr_param_hpp: C++ header file for CSR definitions ({name}_csr_param.hpp)
    - {name}_csr_param_sv: SystemVerilog file for CSR definitions ({name}_csr_param.sv)
    - PMU parameter files: Generated from pmu_spec
    
    The CSR collateral is automatically included in:
    - SystemVerilog modules: {name}_csr_param_sv is added to verilog_library deps
    - C++ modules: {name}_csr_param_hpp is added to cc_library deps
    
    
    Args:
        name: Base name for generated targets
        topology: Topology target 
        csr_spec: CSR specification file
        pmu_spec: PMU specification filegroup containing core and SC CSV files
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
        csr_param_override = csr_param_override,
        package = "csr_param",
        cc_attrs = cc_attrs,
    )

    pmu_fragment_gen(
        name = name + "_pmu_fragments",
        pmu_spec = pmu_spec,
        pmu_template = "@rv_tester//pmu:pmu.sv",
        cc_attrs = cc_attrs,
    )

    verilog_library(
        name = name + "_harness",
        srcs = [
            "@rv_tester//:rv_tester_pkg.sv",
            "@rv_tester//:rv_tester_defines.sv",
            "@rv_tester//:rv_tester_stall_checker.sv",
        ],
        deps = [
            topology + "_sv",
	    "@opensrc-axi//:axi",
            name + "_pmu_fragments_pmu_defines_sv",
        ],
        visibility = visibility,
    )

    packet_gen(
        name = name + "_transactions",
        srcs = [
            name + "_pmu_fragments/gen_core_events.yaml",
            name + "_pmu_fragments/gen_sc_events.yaml",
            "@rv_tester//:rv_tester_transactions.yml",
        ],
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
        csr_param = name + "_csr_param",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    pmu_gen(
        name = name + "_pmu",
        packet = name  + "_transactions",
        topology = topology,
        harness = name + "_harness",
        pmu_spec = pmu_spec,
        pmu_fragments = name + "_pmu_fragments",
        cc_attrs = cc_attrs,
    )

    interrupts_gen(
        name = name + "_interrupts",
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
            "@rv_tester//:rv_tester_delay_resp.sv",
            "@rv_tester//:rv_tester_keeper.sv",
        ],
        deps = [
            "@cvm//:logger_sv",
            name + "_harness",
            name + "_pmu_fragments_pmu_defines_sv",
            name + "_transactions_sv",
            name + "_sysmod_sv",
            name + "_pmu_sv",
            name + "_interrupts_sv",
            name + "_triggers_sv",
            name + "_axi_sw_sv",
            "@opensrc-axi_llc//:axi_llc",
            "@opensrc-axi//:axi",
            "@opensrc-tech_cells_generic//:tech_cells_generic"
        ] 
        + select({
          "@rv_tester//:cosim_off": ["@rv_tester//:no_cosim"],
          "//conditions:default":   [name + "_cosim_sv"],
        })
        ,
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
            name + "_interrupts_dpi",
            name + "_triggers_dpi",
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
        deps = ["@cvm//:logger", "@cvm//:plusargs"],
        alwayslink = True,
        visibility = visibility,
    )
