load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def _csr_param_gen_impl(ctx):
    
    name = ctx.attr.name    
    csr_map_hpp = ctx.outputs.csr_map_hpp
    csr_map_sv = ctx.outputs.csr_map_sv

    args = ctx.actions.args()
    args.add("--csr_spec", ctx.file.csr_spec)
    args.add("--csr_map_hpp", csr_map_hpp)
    args.add("--csr_map_sv", csr_map_sv)

    inputs = [ctx.file.csr_spec]
    outputs = [csr_map_hpp, csr_map_sv]

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
        "csr_map_hpp": attr.output(
            mandatory = True,
            doc = "Output C++ header file",
        ),
        "csr_map_sv": attr.output(
            mandatory = True,
            doc = "Output SystemVerilog defines file",
        ),
        "_csr_param_gen": attr.label(
            default = "@rv_tester//csr:csr_param_gen",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [
        DefaultInfo,
    ],
)

def csr_param_gen(name, csr_spec = "@rv_tester//csr:csr_spec", package = "", visibility = None, cc_attrs = {}, **kwargs):

    csr_map_hpp = name + ".hpp"
    csr_map_sv = name + ".sv"

    if package:
      csr_map_hpp = name + "/" + package + ".hpp"
      csr_map_sv = name + "/" + package + ".sv"

    _csr_param_gen(
        name = name,
        csr_spec = csr_spec,
        csr_map_hpp = csr_map_hpp,
        csr_map_sv = csr_map_sv,
        visibility = visibility,
        **kwargs,
    )

    native.cc_library(
        name = name + '_cc',
        hdrs = [csr_map_hpp],
        visibility = visibility,
        strip_include_prefix = name if package else ".",
        **cc_attrs,
    )

    verilog_library(
        name = name + '_sv',
        srcs = [csr_map_sv],
        visibility = visibility,
    )
