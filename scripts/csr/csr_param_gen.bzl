load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def _csr_param_gen_impl(ctx):

    name = ctx.attr.name
    csr_map_hpp = ctx.outputs.csr_map_hpp
    csr_map_sv = ctx.outputs.csr_map_sv
    csral_tables_hpp = ctx.outputs.csral_tables_hpp

    args = ctx.actions.args()
    args.add("--csr_spec", ctx.file.csr_spec)
    args.add("--csr_map_hpp", csr_map_hpp)
    args.add("--csr_map_sv", csr_map_sv)
    args.add("--csral_defaults", ctx.file.csral_defaults)
    args.add("--csral_tables_hpp", csral_tables_hpp)

    inputs = [ctx.file.csr_spec, ctx.file.csral_defaults]

    if ctx.file.project_override:
        args.add("--project_override", ctx.file.project_override)
        inputs.append(ctx.file.project_override)

    outputs = [csr_map_hpp, csr_map_sv, csral_tables_hpp]

    ctx.actions.run(
        arguments = [args],
        executable = ctx.executable._csr_param_gen,
        inputs = inputs,
        outputs = outputs,
        mnemonic = "CSRParamGen"
    )

    return [
        DefaultInfo(
            files = depset(outputs,)
        ),
    ]

_csr_param_gen = rule(
    _csr_param_gen_impl,
    attrs = {
        "csr_spec": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "Path to CSR specification yaml file",
        ),
        "project_override": attr.label(
            allow_single_file = True,
            mandatory = False,
            doc = "Path to YAML file with CSR parameter overrides",
        ),
        "csral_defaults": attr.label(
            allow_single_file = True,
            default = "@rv_tester//scripts/csr:csral_defaults",
            doc = "CSRAL policy defaults YAML (merged with the project override's csral: section)",
        ),
        "csr_map_hpp": attr.output(
            mandatory = True,
            doc = "Output C++ header file",
        ),
        "csr_map_sv": attr.output(
            mandatory = True,
            doc = "Output SystemVerilog defines file",
        ),
        "csral_tables_hpp": attr.output(
            mandatory = True,
            doc = "Output CSRAL tables C++ header",
        ),
        "_csr_param_gen": attr.label(
            default = "@rv_tester//scripts/csr:csr_param_gen",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [
        DefaultInfo,
    ],
)

def csr_param_gen(name, csr_spec, project_override = None, csral_defaults = None, package = "", visibility = None, cc_attrs = {}, **kwargs):

    csr_map_hpp = name + ".hpp"
    csr_map_sv = name + ".sv"
    csral_tables_hpp = name + "_csral_tables.hpp"

    if package:
      csr_map_hpp = name + "/" + package + ".hpp"
      csr_map_sv = name + "/" + package + ".sv"
      csral_tables_hpp = name + "/csral_tables.hpp"

    _csr_param_gen(
        name = name,
        csr_spec = csr_spec,
        project_override = project_override,
        csral_defaults = csral_defaults or "@rv_tester//scripts/csr:csral_defaults",
        csr_map_hpp = csr_map_hpp,
        csr_map_sv = csr_map_sv,
        csral_tables_hpp = csral_tables_hpp,
        visibility = visibility,
        **kwargs,
    )

    native.cc_library(
        name = name + '_cc',
        hdrs = [csr_map_hpp, csral_tables_hpp],
        visibility = visibility,
        strip_include_prefix = name if package else ".",
        **cc_attrs,
    )

    verilog_library(
        name = name + '_sv',
        srcs = [csr_map_sv],
        visibility = visibility,
    )
