load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def cosim_gen(name, packet, csr_param, topology, harness, project_overrides_cc, visibility = None, cc_attrs = {}, **kwargs):

    cosim_dpi = name + "_dpi"
    cosim_sv = name + "_sv"

    native.cc_library(
        name = name + "_rvfi",
        srcs = [
            "@rv_tester//src/cosim/dut_if/rvfi:rvfi.cpp",
            "@rv_tester//src/cosim/utils/eot:eot.cpp",
            "@rv_tester//src/cosim/utils/sot:sot.cpp",
        ],
        hdrs = [
            "@rv_tester//src/cosim/dut_if/rvfi:rvfi.h",
            "@rv_tester//src/cosim/utils/eot:eot.h",
            "@rv_tester//src/sysmod:sysmod_rpc.h",
            "@rv_tester//src:rv_tester_structs.h",
            "@rv_tester//src/cosim/utils/sot:sot.h",
        ],
        deps = [
            "@rv_tester//src:structs",
            "@rv_tester//src:plusargs",
            "@rv_tester//src:device_handler",
            "@rv_tester//src/cosim/utils/eot:eot_plusargs",
            "@rv_tester//src/cosim/whisper_if:whisper_client_plusargs",
            "@rv_tester//src/cosim/dut_if/rvfi:rvfi_plusargs",
            "@rv_tester//src/cosim/utils/general:util",
            packet + "_cc",
            name + "_bridge",
            name + "_mcmi",
            "@rv_tester//src/sysmod/htif:htif",
            "@rv_tester//src/cosim/whisper_if:whisper_decoder",
            "@rv_tester//src/transactors/axi_sw:axi_sw_h",
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
        name = name + "_mcmi",
        srcs = [
            "@rv_tester//src/cosim/dut_if/mcmi:mcmi.cpp",
        ],
        hdrs = [
            "@rv_tester//src/cosim/dut_if/mcmi:mcmi.h",
        ],
        deps = [
            "@rv_tester//src:structs",
            "@rv_tester//src:plusargs",
            name + "_bridge",
            "@rv_tester//src/cosim/whisper_if:whisper_client_plusargs",
            "@rv_tester//src/cosim/utils/general:util",
            packet + "_cc",
        ],
        alwayslink = True,
        visibility = visibility,
    )

    native.cc_library(
    name     = name + "_bridge",
    hdrs     = [
                "@rv_tester//src/cosim/bridge:bridge_base.h",
                "@rv_tester//src/cosim/bridge:bridge.h",
    ],
    srcs     = [
                "@rv_tester//src/cosim/bridge:bridge.cpp",
    ],
     deps    = [
                csr_param + "_cc",
                "@rv_tester//src/common:parser",
                "@rv_tester//src:structs",
                "@rv_tester//src:device_handler",
                "@rv_tester//src/cosim/utils/general:util",
                "@rv_tester//src/cosim/whisper_if:whisper_if",
                "@rv_tester//src/cosim/whisper_if:whisper_decoder",
                "@rv_tester//src/cosim/bridge:bridge_if",
                project_overrides_cc,
                "@rv_tester//src/common:common",
                "@CoreArchChecker//src:cac_core",
                "@CoreArchChecker//src:cac_lib",
                "@cvm//:plusargs",
                "@cvm//:logger",
                "@cvm//:random",
                "@cvm//:topology",
                "@fmt//:fmt",
                "@rv_tester//src/sysmod/htif:htif",
                "@rv_tester//src:plusargs",
                "@rv_tester//src/cosim/dut_if/rvfi:rvfi_plusargs",
                "@rv_tester//src/cosim/utils/eot:eot_plusargs",
                "@rv_tester//src/sysmod:sysmod_plusargs",
                "@rv_tester//src/sysmod:sysmod_params",
                "@rv_tester//src/sysmod/trickbox:trickbox",
               ],
    data     = [
                "@whisper//:whisper",
               ],
    alwayslink = True,
    visibility = ["//visibility:public"],
)

    verilog_library(
        name = cosim_sv,
        srcs = ["@rv_tester//src/cosim:cosim.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            "@cvm//:registry_sv",
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
            packet + "_cc",
         ],
        alwayslink = True,
        visibility = visibility,
    )
