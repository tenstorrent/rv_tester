load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@rules_hdl//verilog:build_defs.bzl", "verilog_flist")

def axi_sw_gen(name, packet, visibility = None, cc_attrs = {}, **kwargs):

    axi_sw_dpi = name + "_dpi"
    axi_sw_sv = name + "_sv"

    verilog_library(
        name = axi_sw_sv,
        srcs = [
            "@rv_tester//transactors/axi_sw:axi_sw.sv"
        ],
        deps = [
            "@rv_tester//:rv_tester_lib",
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            "@cvm//:random_sv",
            packet + "_sv",
        ],
        visibility = ["//visibility:public"],
    )

    verilog_flist(
        name = name + "_f",
        srcs = [
            axi_sw_sv
        ],
    )

    native.cc_library(
        name = axi_sw_dpi,
        srcs = [
            "@rv_tester//transactors/axi_sw:axi_sw.cpp",
            "@rv_tester//transactors/axi_sw:axi_sw_mst.cpp",
            "@rv_tester//transactors/axi_sw:axi.cpp",
        ],
        hdrs = [
            "@rv_tester//transactors/axi_sw:axi_sw.h",
            "@rv_tester//transactors/axi_sw:axi_sw_mst.h",
            "@rv_tester//transactors/axi_sw:axi.h",
            "@rv_tester//transactors/axi_sw:safe_queue.h",
        ],
        deps = [
          "@rv_tester//common:common",
          "@rv_tester//:plusargs",
          "@cvm//:plusargs",
          "@cvm//:topology",
          "@cvm//:registry",
          "@cvm//:logger",
          "@cvm//:bitmanip",
          "@cvm//:messenger",
          packet + "_cc",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
