load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def trace_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    trace_dpi = name + "_dpi"
    trace_sv = name + "_sv"

    native.cc_library(
        name = trace_dpi,
        srcs = [
            "@rv_tester//trace:trace.cpp",
            "@rv_tester//trace:ntrace_stop_on_wrap_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//trace:trace.hpp",
            "@rv_tester//trace:ntrace_stop_on_wrap_sequence.hpp",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//common:transactor",
            "@rv_tester//:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:random",
            "@cvm//:logger",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = trace_sv,
        srcs = ["@rv_tester//trace:trace.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
