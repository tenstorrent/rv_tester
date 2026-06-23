load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def _pmu_fragment_gen_impl(ctx):
    name = ctx.attr.name

    # Extract and validate CSV files from pmu_spec
    pmu_spec_files = ctx.files.pmu_spec
    core_pmc_csv = None
    sc_pmc_csv = None
    for f in pmu_spec_files:
        if "core" in f.basename:
            core_pmc_csv = f
        elif "sharedcache" in f.basename:
            sc_pmc_csv = f

    # Consolidated validation with clear error message
    errors = []
    if len(pmu_spec_files) != 2:
        errors.append("pmu_spec must contain exactly 2 CSV files, found %d" % len(pmu_spec_files))
    if not core_pmc_csv:
        errors.append("missing core_pmc_spec.csv (file with 'core' in name)")
    if not sc_pmc_csv:
        errors.append("missing sharedcache_pmc_spec.csv (file with 'sharedcache' in name)")
    if errors:
        fail("pmu_fragment_gen '%s' validation failed:\n  - %s" % (name, "\n  - ".join(errors)))

    # Get pmu_template if provided
    pmu_template_files = ctx.files.pmu_template if ctx.attr.pmu_template else []

    # Define all output files
    gen_events_core_hpp = ctx.outputs.gen_events_core_hpp
    gen_events_sc_hpp = ctx.outputs.gen_events_sc_hpp
    gen_core_events_svh = ctx.outputs.gen_core_events_svh
    gen_sc_events_svh = ctx.outputs.gen_sc_events_svh
    gen_pmu_core_pkg_sv = ctx.outputs.gen_pmu_core_pkg_sv
    gen_pmu_sc_pkg_sv = ctx.outputs.gen_pmu_sc_pkg_sv
    gen_core_monitor_svh = ctx.outputs.gen_core_monitor_svh
    gen_sc_monitor_svh = ctx.outputs.gen_sc_monitor_svh
    gen_core_events_yaml = ctx.outputs.gen_core_events_yaml
    gen_sc_events_yaml = ctx.outputs.gen_sc_events_yaml
    gen_pmu_sv = ctx.outputs.gen_pmu_sv

    args = ctx.actions.args()
    args.add("--core_pmc_csv", core_pmc_csv)
    args.add("--sc_pmc_csv", sc_pmc_csv)
    args.add("--gen_events_core_hpp", gen_events_core_hpp)
    args.add("--gen_events_sc_hpp", gen_events_sc_hpp)
    args.add("--gen_core_events_sv", gen_core_events_svh)
    args.add("--gen_sc_events_sv", gen_sc_events_svh)
    args.add("--gen_pmu_core_pkg_sv", gen_pmu_core_pkg_sv)
    args.add("--gen_pmu_sc_pkg_sv", gen_pmu_sc_pkg_sv)
    args.add("--gen_core_monitor_sv", gen_core_monitor_svh)
    args.add("--gen_sc_monitor_sv", gen_sc_monitor_svh)
    args.add("--gen_core_events_yaml", gen_core_events_yaml)
    args.add("--gen_sc_events_yaml", gen_sc_events_yaml)

    inputs = [core_pmc_csv, sc_pmc_csv]
    outputs = [
        gen_events_core_hpp, gen_events_sc_hpp,
        gen_core_events_svh, gen_sc_events_svh,
        gen_pmu_core_pkg_sv, gen_pmu_sc_pkg_sv,
        gen_core_monitor_svh, gen_sc_monitor_svh,
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
        "gen_core_events_svh": attr.output(mandatory = True),
        "gen_sc_events_svh": attr.output(mandatory = True),
        "gen_pmu_core_pkg_sv": attr.output(mandatory = True),
        "gen_pmu_sc_pkg_sv": attr.output(mandatory = True),
        "gen_core_monitor_svh": attr.output(mandatory = True),
        "gen_sc_monitor_svh": attr.output(mandatory = True),
        "gen_core_events_yaml": attr.output(mandatory = True),
        "gen_sc_events_yaml": attr.output(mandatory = True),
        "gen_pmu_sv": attr.output(mandatory = False),
        "_pmu_gen": attr.label(
            default = "@rv_tester//src/pmu:pmu_gen",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [DefaultInfo],
)

def pmu_fragment_gen(name, pmu_spec, package = "", pmu_template = None, visibility = None, cc_attrs = {}, **kwargs):
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
    # `gen_*_events` and `gen_*_monitor` are include-fragments (no
    # module/endmodule wrapper, just bodies meant to be `\`include`d inside a
    # surrounding module). Name them `.svh` so hw-bzl rules_verilator's CLI
    # filter drops them from the Verilator command line; they still reach the
    # sandbox via the verilog_library `srcs` depset and resolve via `-I<dir>`
    # search paths emitted by the rules_hdl_compat shim.
    gen_core_events_svh = name + "/gen_core_events.svh"
    gen_sc_events_svh = name + "/gen_sc_events.svh"
    gen_pmu_core_pkg_sv = name + "/gen_pmu_core_pkg.sv"
    gen_pmu_sc_pkg_sv = name + "/gen_pmu_sc_pkg.sv"
    gen_core_monitor_svh = name + "/gen_core_monitor.svh"
    gen_sc_monitor_svh = name + "/gen_sc_monitor.svh"
    gen_core_events_yaml = name + "/gen_core_events.yaml"
    gen_sc_events_yaml = name + "/gen_sc_events.yaml"
    gen_pmu_sv = name + "/pmu.sv" if pmu_template else None

    _pmu_fragment_gen(
        name = name,
        pmu_spec = pmu_spec,
        pmu_template = pmu_template,
        gen_events_core_hpp = gen_events_core_hpp,
        gen_events_sc_hpp = gen_events_sc_hpp,
        gen_core_events_svh = gen_core_events_svh,
        gen_sc_events_svh = gen_sc_events_svh,
        gen_pmu_core_pkg_sv = gen_pmu_core_pkg_sv,
        gen_pmu_sc_pkg_sv = gen_pmu_sc_pkg_sv,
        gen_core_monitor_svh = gen_core_monitor_svh,
        gen_sc_monitor_svh = gen_sc_monitor_svh,
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
            gen_core_events_svh,
            gen_sc_events_svh,
            gen_core_monitor_svh,
            gen_sc_monitor_svh,
        ],
        deps = [name + '_pmu_defines_sv'],
        visibility = visibility,
    )

    if package:
        gen_pmu_core_pkg_sv = name + "/" + package + "_core_pkg.sv"
        gen_pmu_sc_pkg_sv = name + "/" + package + "_sc_pkg.sv"

    verilog_library(
        name = name + '_pmu_defines_sv',
        srcs = [
            gen_pmu_core_pkg_sv,
            gen_pmu_sc_pkg_sv,
        ],
        visibility = visibility,
    )