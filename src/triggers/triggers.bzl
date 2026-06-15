load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def triggers_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    triggers_dpi = name + "_dpi"
    triggers_sv = name + "_sv"

    native.cc_library(
        name = triggers_dpi,
        srcs = [
            "@rv_tester//src/triggers:triggers.cpp",
        ],
        hdrs = [
            "@rv_tester//src/triggers:triggers.hpp",
        ],
        deps = [
            "@rv_tester//src/sysmod:sysmod_plusargs",
            "@rv_tester//src/common:transactor",
            "@rv_tester//src:structs",
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
        name = triggers_sv,
        srcs = ["@rv_tester//src/triggers:triggers.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            "@cvm//:registry_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
