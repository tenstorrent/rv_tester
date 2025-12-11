load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def pwrmgmt_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    pwrmgmt_dpi = name + "_dpi"
    pwrmgmt_sv = name + "_sv"

    native.cc_library(
        name = pwrmgmt_dpi,
        srcs = [
            "@rv_tester//pwrmgmt:pwrmgmt.cpp",
            "@rv_tester//pwrmgmt:reset_sequence.cpp",
            "@rv_tester//pwrmgmt:patch_control_sequence.cpp",
            "@rv_tester//pwrmgmt:dfs_sequence.cpp",
            "@rv_tester//pwrmgmt:thub_sequence.cpp",
            "@rv_tester//pwrmgmt:patch_utils.cpp",
        ],
        hdrs = [
            "@rv_tester//pwrmgmt:pwrmgmt.hpp",
            "@rv_tester//pwrmgmt:reset_sequence.hpp",
            "@rv_tester//pwrmgmt:patch_control_sequence.hpp",
            "@rv_tester//pwrmgmt:dfs_sequence.hpp",
            "@rv_tester//pwrmgmt:thub_sequence.hpp",
            "@rv_tester//pwrmgmt:patch_utils.hpp",
        ],
        deps = [
            "@rv_tester//transactors/axi_sw:axi_sw_mst",
            "@rv_tester//:plusargs",
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//pmu:pmu_plusargs",
            "@rv_tester//common:transactor",
            "@rv_tester//cosim/bridge_if:bridge_if",
            "@rv_tester//cosim/utils/general:util",
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
        name = pwrmgmt_sv,
        srcs = [
            "@rv_tester//pwrmgmt:pwrmgmt.sv",
        ],
        deps = [
            "@rv_tester//:rv_tester_tick_generator",
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
