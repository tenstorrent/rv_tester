load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def sysmod_gen(name, packet, csr_param, topology, project_overrides_cc, visibility = None, cc_attrs = {}, **kwargs):

    sysmod_dpi = name + "_dpi"
    sysmod_sv = name + "_sv"


    verilog_library(
        name = sysmod_sv,
        srcs = ["@rv_tester//sysmod:sysmod.sv"],
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
          "@rv_tester//sysmod:sysmod.cpp",
        ],
        hdrs = [
          "@rv_tester//sysmod:sysmod.h",
          "@rv_tester//sysmod:sysmod_rpc.h"
        ],
        deps = [
          "@rv_tester//:plusargs",
          "@rv_tester//:structs",
          "@rv_tester//common:common",
          "@rv_tester//sysmod:device",
          "@rv_tester//sysmod/clint:clint",
          "@rv_tester//sysmod/aclint:aclint",
          "@rv_tester//sysmod/trickbox:trickbox",
          "@rv_tester//sysmod/sep_entropy_fifo:sep_entropy_fifo",
          "@rv_tester//sysmod/htif:htif",
          "@rv_tester//sysmod/aplic:aplic_device",
          "@rv_tester//sysmod/mem:mem",
          "@rv_tester//sysmod/dm:dm",
          "@rv_tester//sysmod/io_dev:io_dev",
          "@rv_tester//sysmod/mmr_txn_router:mmr_txn_router",
          "@rv_tester//sysmod/null_dev:null_dev",
          "@rv_tester//sysmod/heartbeat:heartbeat",
          "@rv_tester//cosim/bridge:bridge_if",
          project_overrides_cc,
          "@rv_tester//cosim/bridge:bridge_plusargs",
          "@rv_tester//cosim/dut_if/rvfi:rvfi_plusargs",
          "@rv_tester//cosim/whisper_if:whisper_client_plusargs",
          "@rv_tester//cosim/utils/general:util",
          "@rv_tester//pmu:pmu_plusargs",
          "@cvm//:plusargs",
          "@cvm//:topology",
          "@cvm//:registry",
          "@rv_tester//sysmod:sysmod_params",
          packet + "_cc",
          csr_param + "_cc",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
