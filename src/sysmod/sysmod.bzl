load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def sysmod_gen(name, packet, csr_param, topology, project_overrides_cc, visibility = None, cc_attrs = {}, **kwargs):

    sysmod_dpi = name + "_dpi"
    sysmod_sv = name + "_sv"


    verilog_library(
        name = sysmod_sv,
        srcs = ["@rv_tester//src/sysmod:sysmod.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            "@cvm//:registry_sv",
            packet + "_sv",
            topology + "_sv",
        ],
        visibility = ["//visibility:public"],
    )


    native.cc_library(
        name = sysmod_dpi,
        srcs = [
          "@rv_tester//src/sysmod:sysmod.cpp",
        ],
        hdrs = [
          "@rv_tester//src/sysmod:sysmod.h",
          "@rv_tester//src/sysmod:sysmod_rpc.h"
        ],
        deps = [
          "@rv_tester//src:plusargs",
          "@rv_tester//src:structs",
          "@rv_tester//src/common:common",
          "@rv_tester//src/sysmod:device",
          "@rv_tester//src/sysmod/clint:clint",
          "@rv_tester//src/sysmod/aclint:aclint",
          "@rv_tester//src/sysmod/trickbox:trickbox",
          "@rv_tester//src/sysmod/sep_entropy_fifo:sep_entropy_fifo",
          "@rv_tester//src/sysmod/htif:htif",
          "@rv_tester//src/sysmod/aplic:aplic_device",
          "@rv_tester//src/sysmod/mem:mem",
          "@rv_tester//src/sysmod/dm:dm",
          "@rv_tester//src/sysmod/io_dev:io_dev",
          "@rv_tester//src/sysmod/mmr_txn_router:mmr_txn_router",
          "@rv_tester//src/sysmod/null_dev:null_dev",
          "@rv_tester//src/sysmod/heartbeat:heartbeat",
          "@rv_tester//src/cosim/bridge:bridge_if",
          project_overrides_cc,
          "@rv_tester//src/cosim/bridge:bridge_plusargs",
          "@rv_tester//src/cosim/dut_if/rvfi:rvfi_plusargs",
          "@rv_tester//src/cosim/whisper_if:whisper_client_plusargs",
          "@rv_tester//src/cosim/utils/general:util",
          "@rv_tester//src/pmu:pmu_plusargs",
          "@cvm//:plusargs",
          "@cvm//:topology",
          "@cvm//:registry",
          "@rv_tester//src/sysmod:sysmod_params",
          packet + "_cc",
          csr_param + "_cc",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
