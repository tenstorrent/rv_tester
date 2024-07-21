load("@rules_hdl//verilog:providers.bzl", "VerilogInfo")

def _pwrmgmt_on_impl(settings, attr):
    return [
        {"@rv_tester//:pwrmgmt" : True},
    ]

pwrmgmt_on = transition(
    implementation = _pwrmgmt_on_impl,
    inputs = [],
    outputs = ["@rv_tester//:pwrmgmt"]
)

def _rv_tester_with_pwrmgmt_on_sv_impl(ctx):
    return [ctx.attr.dep[0][VerilogInfo]]

rv_tester_with_pwrmgmt_on_sv = rule(
    implementation = _rv_tester_with_pwrmgmt_on_sv_impl,
    attrs = {
        "dep": attr.label(cfg = pwrmgmt_on),
        "_allowlist_function_transition": attr.label(
         default = "@bazel_tools//tools/allowlists/function_transition_allowlist"
     ),
    },
)

def _rv_tester_with_pwrmgmt_on_dpi_impl(ctx):
    return [ctx.attr.dep[0][CcInfo]]

rv_tester_with_pwrmgmt_on_dpi = rule(
    implementation = _rv_tester_with_pwrmgmt_on_dpi_impl,
    attrs = {
        "dep": attr.label(cfg = pwrmgmt_on),
        "_allowlist_function_transition": attr.label(
         default = "@bazel_tools//tools/allowlists/function_transition_allowlist"
     ),
    },
)
