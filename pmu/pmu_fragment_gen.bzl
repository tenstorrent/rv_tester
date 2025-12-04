load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def _pmu_fragment_gen_impl(ctx):
    name = ctx.attr.name

    # Extract the two CSV files from pmu_spec
    pmu_spec_files = ctx.files.pmu_spec
    if len(pmu_spec_files) != 2:
        fail("pmu_spec must contain exactly 2 CSV files (core and sc)")

    # Identify core and sc CSV files by name
    core_pmc_csv = None
    sc_pmc_csv = None
    for f in pmu_spec_files:
        if "core" in f.basename:
            core_pmc_csv = f
        elif "sc" in f.basename:
            sc_pmc_csv = f

    if not core_pmc_csv or not sc_pmc_csv:
        fail("pmu_spec must contain core_pmc_spec.csv and sc_pmc_spec.csv")

    # Get pmu_template if provided
    pmu_template_files = ctx.files.pmu_template if ctx.attr.pmu_template else []

    # Define all output files
    gen_events_core_hpp = ctx.outputs.gen_events_core_hpp
    gen_events_sc_hpp = ctx.outputs.gen_events_sc_hpp
    gen_core_events_sv = ctx.outputs.gen_core_events_sv
    gen_sc_events_sv = ctx.outputs.gen_sc_events_sv
    gen_core_defines_sv = ctx.outputs.gen_core_defines_sv
    gen_sc_defines_sv = ctx.outputs.gen_sc_defines_sv
    gen_core_monitor_sv = ctx.outputs.gen_core_monitor_sv
    gen_sc_monitor_sv = ctx.outputs.gen_sc_monitor_sv
    gen_core_events_yaml = ctx.outputs.gen_core_events_yaml
    gen_sc_events_yaml = ctx.outputs.gen_sc_events_yaml
    gen_pmu_sv = ctx.outputs.gen_pmu_sv

    args = ctx.actions.args()
    args.add("--core_pmc_csv", core_pmc_csv)
    args.add("--sc_pmc_csv", sc_pmc_csv)
    args.add("--gen_events_core_hpp", gen_events_core_hpp)
    args.add("--gen_events_sc_hpp", gen_events_sc_hpp)
    args.add("--gen_core_events_sv", gen_core_events_sv)
    args.add("--gen_sc_events_sv", gen_sc_events_sv)
    args.add("--gen_core_defines_sv", gen_core_defines_sv)
    args.add("--gen_sc_defines_sv", gen_sc_defines_sv)
    args.add("--gen_core_monitor_sv", gen_core_monitor_sv)
    args.add("--gen_sc_monitor_sv", gen_sc_monitor_sv)
    args.add("--gen_core_events_yaml", gen_core_events_yaml)
    args.add("--gen_sc_events_yaml", gen_sc_events_yaml)

    inputs = [core_pmc_csv, sc_pmc_csv]
    outputs = [
        gen_events_core_hpp, gen_events_sc_hpp,
        gen_core_events_sv, gen_sc_events_sv,
        gen_core_defines_sv, gen_sc_defines_sv,
        gen_core_monitor_sv, gen_sc_monitor_sv,
        gen_core_events_yaml, gen_sc_events_yaml,
    ]

    # Add pmu_template args if provided
    if pmu_template_files:
        args.add("--pmu_template_sv", pmu_template_files[0])
        args.add("--gen_pmu_sv", gen_pmu_sv)
        inputs.append(pmu_template_files[0])
        outputs.append(gen_pmu_sv)

    ctx.actions.run(
        arguments = [args],
        executable = ctx.executable._pmu_gen,
        inputs = inputs,
        outputs = outputs,
        mnemonic = "PMUFragmentGen"
    )

    return [DefaultInfo(files = depset(outputs))]

_pmu_fragment_gen = rule(
    _pmu_fragment_gen_impl,
    attrs = {
        "pmu_spec": attr.label(
            mandatory = True,
            doc = "PMU specification filegroup containing core_pmc_spec.csv and sc_pmc_spec.csv",
        ),
        "pmu_template": attr.label(
            mandatory = False,
            allow_files = [".sv"],
            doc = "Optional pmu.sv template file with include placeholders",
        ),
        "gen_events_core_hpp": attr.output(mandatory = True),
        "gen_events_sc_hpp": attr.output(mandatory = True),
        "gen_core_events_sv": attr.output(mandatory = True),
        "gen_sc_events_sv": attr.output(mandatory = True),
        "gen_core_defines_sv": attr.output(mandatory = True),
        "gen_sc_defines_sv": attr.output(mandatory = True),
        "gen_core_monitor_sv": attr.output(mandatory = True),
        "gen_sc_monitor_sv": attr.output(mandatory = True),
        "gen_core_events_yaml": attr.output(mandatory = True),
        "gen_sc_events_yaml": attr.output(mandatory = True),
        "gen_pmu_sv": attr.output(mandatory = False),
        "_pmu_gen": attr.label(
            default = "@rv_tester//pmu:pmu_gen",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [DefaultInfo],
)

def pmu_fragment_gen(name, pmu_spec, pmu_template = None, visibility = None, cc_attrs = {}, **kwargs):
    """Generate PMU fragment files from CSV specifications.

    Args:
        name: Base name for generated targets
        pmu_spec: PMU specification filegroup containing core and SC CSV files
        pmu_template: Optional pmu.sv template file with include placeholders
        visibility: Target visibility
        cc_attrs: C++ compilation attributes
    """

    gen_events_core_hpp = name + "/gen_events_core.hpp"
    gen_events_sc_hpp = name + "/gen_events_sc.hpp"
    gen_core_events_sv = name + "/gen_core_events.sv"
    gen_sc_events_sv = name + "/gen_sc_events.sv"
    gen_core_defines_sv = name + "/gen_core_defines.sv"
    gen_sc_defines_sv = name + "/gen_sc_defines.sv"
    gen_core_monitor_sv = name + "/gen_core_monitor.sv"
    gen_sc_monitor_sv = name + "/gen_sc_monitor.sv"
    gen_core_events_yaml = name + "/gen_core_events.yaml"
    gen_sc_events_yaml = name + "/gen_sc_events.yaml"
    gen_pmu_sv = name + "/pmu.sv" if pmu_template else None

    _pmu_fragment_gen(
        name = name,
        pmu_spec = pmu_spec,
        pmu_template = pmu_template,
        gen_events_core_hpp = gen_events_core_hpp,
        gen_events_sc_hpp = gen_events_sc_hpp,
        gen_core_events_sv = gen_core_events_sv,
        gen_sc_events_sv = gen_sc_events_sv,
        gen_core_defines_sv = gen_core_defines_sv,
        gen_sc_defines_sv = gen_sc_defines_sv,
        gen_core_monitor_sv = gen_core_monitor_sv,
        gen_sc_monitor_sv = gen_sc_monitor_sv,
        gen_core_events_yaml = gen_core_events_yaml,
        gen_sc_events_yaml = gen_sc_events_yaml,
        gen_pmu_sv = gen_pmu_sv,
        visibility = visibility,
        **kwargs,
    )

    native.cc_library(
        name = name + '_cc',
        hdrs = [
            gen_events_core_hpp,
            gen_events_sc_hpp,
        ],
        visibility = visibility,
        strip_include_prefix = name,
        **cc_attrs,
    )

    # Create verilog_library with generated pmu.sv if template was provided
    sv_srcs = [gen_pmu_sv] if gen_pmu_sv else []
    verilog_library(
        name = name + '_sv',
        srcs = sv_srcs,
        hdrs = [
            gen_core_events_sv,
            gen_sc_events_sv,
            gen_core_defines_sv,
            gen_sc_defines_sv,
            gen_core_monitor_sv,
            gen_sc_monitor_sv,
        ],
        visibility = visibility,
    )
