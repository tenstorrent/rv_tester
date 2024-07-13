load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def pwrmgmt_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    pwrmgmt_dpi = name + "_dpi"
    pwrmgmt_sv = name + "_sv"

    native.cc_library(
        name = pwrmgmt_dpi,
        srcs = [
            "@rv_tester//pwrmgmt:pwrmgmt.cpp",
            "@rv_tester//pwrmgmt:reset_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//pwrmgmt:pwrmgmt.hpp",
            "@rv_tester//pwrmgmt:reset_sequence.hpp",
        ],
        deps = [
            "@rv_tester//common:transactor",
            "@rv_tester//common:rand_gflags",
            "@rv_tester//:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:plusargs_dpi",
            "@cvm//:logger",
            "@cvm//:registry",
            "@rv_tester//:plusargs"
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = pwrmgmt_sv,
        srcs = ["@rv_tester//pwrmgmt:pwrmgmt.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
