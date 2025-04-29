load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def cosim_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    cosim_dpi = name + "_dpi"
    cosim_sv = name + "_sv"

    native.cc_library(
        name = name + "_rvfi",
        srcs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.cpp",
            "@rv_tester//cosim/utils/eot:eot.cpp",
            "@rv_tester//cosim/utils/sot:sot.cpp",
        ],
        hdrs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.h",
            "@rv_tester//cosim/utils/eot:eot.h",
            "@rv_tester//cosim/utils/sot:sot.h",
        ],
        deps = [
            "@rv_tester//:structs",
            "@rv_tester//cosim/utils/eot:eot_plusargs",
            "@rv_tester//cosim/utils/general:util",
            packet + "_cc",
            "@rv_tester//sysmod/htif:htif",
            "@rv_tester//cosim/bridge_if:bridge_if",
            "@rv_tester//cosim/bridge:bridge",
            "@rv_tester//cosim/whisper_if:whisper_decoder",
            "@fmt//:fmt",
            "@cvm//:plusargs",
            "@cvm//:logger",
            "@cvm//:bitmanip",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    verilog_library(
        name = cosim_sv,
        srcs = ["@rv_tester//cosim:cosim.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            "@rv_tester//cosim/whisper_cov:archcov_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )

    native.cc_library(
        name = cosim_dpi,
        deps = [
            name + "_rvfi",
            "@cvm//:plusargs",
            "@rv_tester//cosim/whisper_cov:archsample_dpi",
            packet + "_cc",
         ],
        alwayslink = True,
        visibility = visibility,
    )
