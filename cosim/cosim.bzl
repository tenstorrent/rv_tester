load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def cosim_gen(name, packet, csr_collateral, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

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
            "@rv_tester//sysmod:sysmod_rpc.h",
            "@rv_tester//:rv_tester_structs.h",
            "@rv_tester//cosim/utils/sot:sot.h",
        ],
        deps = [
            "@rv_tester//:structs",
            "@rv_tester//:plusargs",
            "@rv_tester//cosim/utils/eot:eot_plusargs",
            "@rv_tester//cosim/whisper_if:whisper_client_plusargs",
            "@rv_tester//cosim/dut_if/rvfi:rvfi_plusargs",
            "@rv_tester//cosim/utils/general:util",
            packet + "_cc",
            name + "_bridge",
            "@rv_tester//sysmod/htif:htif",
            "@rv_tester//cosim/whisper_if:whisper_decoder",
            "@rv_tester//transactors/axi_sw:axi",
            "@fmt//:fmt",
            "@cvm//:plusargs",
            "@cvm//:logger",
            "@cvm//:bitmanip",
            "@cvm//:registry",
         ],
        alwayslink = True,
        visibility = visibility,
    )

    native.cc_library(
    name     = name + "_bridge",
    hdrs     = [
                "@rv_tester//cosim/bridge:bridge_base.h",
                "@rv_tester//cosim/bridge:bridge.h",
    ],
    srcs     = [
                "@rv_tester//cosim/bridge:bridge.cpp",
    ],
     deps    = [
                csr_collateral + "_cc",
                "@rv_tester//common:parser",
                "@rv_tester//:structs",
                "@rv_tester//cosim/utils/general:util",
                "@rv_tester//cosim/whisper_if:whisper_if",
                "@rv_tester//cosim/whisper_if:whisper_decoder",
                "@rv_tester//cosim/bridge_if:bridge_if",
                "@rv_tester//common:common",
                "@CoreArchChecker//src:cac_core",
                "@CoreArchChecker//src:cac_lib",
                "@cvm//:plusargs",
                "@cvm//:logger",
                "@cvm//:random",
                "@cvm//:topology",
                "@fmt//:fmt",
                "@rv_tester//sysmod/htif:htif",
                "@rv_tester//:plusargs",
                "@rv_tester//cosim/dut_if/rvfi:rvfi_plusargs",
                "@rv_tester//cosim/utils/eot:eot_plusargs",
                "@rv_tester//sysmod:sysmod_plusargs",
                "@rv_tester//sysmod:sysmod_params",
                "@rv_tester//sysmod/trickbox:trickbox",
               ],
    data     = [
                "@whisper//:whisper",
               ],
    alwayslink = True,
    visibility = ["//visibility:public"],
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