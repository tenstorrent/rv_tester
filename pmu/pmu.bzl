load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def pmu_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    pmu_dpi = name + "_dpi"
    pmu_sv = name + "_sv"

    native.cc_library(
        name = pmu_dpi,
        srcs = [
            "@rv_tester//pmu:pmu.cpp"
        ],
        hdrs = [
            "@rv_tester//pmu:pmu.hpp",
        ],
        deps = [
            "@rv_tester//:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:logger",
            "@cvm//:bitmanip",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = pmu_sv,
        srcs = ["@rv_tester//pmu:pmu.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
