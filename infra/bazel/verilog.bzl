# Used to wrap @bzsim's verilog.bzl with defaults for ascalon

load("@bzsim//:verilog_unit.bzl", bzsim_verilog_unit ="verilog_unit")
load("@bzsim//:verilog_unit.bzl", bzsim_verilog_dpi_library ="verilog_dpi_library")
load("@bzsim//:verilog_unit.bzl", bzsim_verilog_library ="verilog_library")
load("//rtl/gen_files:flavors.bzl", "DEFAULT", "FLAVOR_TO_SC_CCHI_FLAVOR", "FLAVORS")


def verilog_unit(
    name, 
    top,
    lib = None,
    runtime = "@risc-p-cores//infra/bzsim-config:risc_p_cores_runtime",
    bzsimconfig         = "//infra/bzsim-config:bzsimconfig",
    vcs_message_configs = ["@rv-common//rtl/gen_files:vcs_tsmc_waiver", "@risc-p-cores//dv/core:vcs_lint_rules"],
    verible_lint_config = "@risc-p-cores//dv/core:verible-lint-rules.config",
    verilator_configs   = ["@risc-p-cores//dv/core:verilator-config.vlt", "@rv-common//rtl/gen_files:verilator_tsmc_waiver"],
    spyglass_lint_tcl   = ["@risc-p-cores//infra/lint/spyglass:spyglass.tcl"],
    risc_p_cores_flavor = None,
    sc_cchi_flavor      = None,
    views               = [],
    **kwargs
    ):

    flavors_map = {name: risc_p_cores_flavor}

    olib = lib or name

    if risc_p_cores_flavor == None:
        flavors_map[name] = DEFAULT
        for flavor in FLAVORS:
            flavors_map[name + "_" + flavor] = flavor

    for name,risc_p_cores_flavor in flavors_map.items():
        bzsim_verilog_unit( 
            name                = name,
            lib                 = olib,
            top                 = top,
            runtime_library     = runtime, 
            bzsimconfig         = bzsimconfig,
            vcs_message_configs = vcs_message_configs,
            verible_lint_config = verible_lint_config,
            verilator_configs   = verilator_configs,
            spyglass_lint_tcl   = spyglass_lint_tcl,
            views               = views + [
                "risc-p-cores-{}".format(risc_p_cores_flavor),
                "rv-common-sc_cchi-{}".format(sc_cchi_flavor or FLAVOR_TO_SC_CCHI_FLAVOR[risc_p_cores_flavor]),
            ],
            **kwargs
        )


def verilog_library(**kwargs):
    bzsim_verilog_library(**kwargs)

def verilog_dpi_library(deps = [], **kwargs):
    bzsim_verilog_dpi_library(deps, **kwargs)
