# Used to wrap @bzsim's verilog.bzl with defaults for ascalon

load("@bzsim//:verilog_unit.bzl", bzsim_verilog_unit ="verilog_unit")
load("@bzsim//:verilog_unit.bzl", bzsim_verilog_dpi_library ="verilog_dpi_library")
load("@bzsim//:verilog_unit.bzl", bzsim_verilog_library ="verilog_library")


def verilog_unit(
    name, 
    top,
    runtime = "//infra/bzsim-config:chips_runtime",
    vcs_message_configs = ["@rv-common//rtl/gen_files:vcs_tsmc_waiver", "@risc-p-cores//dv/core:vcs_lint_rules"],
    verible_lint_config = "@risc-p-cores//dv/core:verible-lint-rules.config",
    verilator_configs   = ["@risc-p-cores//dv/core:verilator-config.vlt", "@rv-common//rtl/gen_files:verilator_tsmc_waiver"],
    spyglass_lint_tcl   = ["@risc-p-cores//infra/lint/spyglass:spyglass.tcl"],
    **kwargs
    ):

    bzsim_verilog_unit( 
        name                = name, 
        top                 = top,
        runtime_library     = runtime, 
        bzsimconfig         = "//infra/bzsim-config:chips_bzsimconfig",
        vcs_message_configs = vcs_message_configs,
        verible_lint_config = verible_lint_config,
        verilator_configs   = verilator_configs,
        spyglass_lint_tcl   = spyglass_lint_tcl,
        **kwargs
    )

def verilog_library(**kwargs):
    bzsim_verilog_library(**kwargs)

def verilog_dpi_library(deps = [], **kwargs):
    bzsim_verilog_dpi_library(deps, **kwargs)
