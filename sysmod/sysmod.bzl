load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def sysmod_gen(name, packet, topology, visibility = None, cc_attrs = {}, **kwargs):

    sysmod_dpi = name + "_dpi"
    sysmod_sv = name + "_sv"

    verilog_library(
        name = sysmod_sv,
        srcs = ["@rv_tester//sysmod:sysmod.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
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
          "@rv_tester//sysmod:sysmod.h"
        ],
        deps = [
          "@rv_tester//common:common",
          "@rv_tester//sysmod:device",
          "@rv_tester//sysmod/clint:clint",
          "@rv_tester//sysmod/trickbox:trickbox",
          "@rv_tester//sysmod/htif:htif",
          "@rv_tester//sysmod/mem:mem",
          "@rv_tester//sysmod/io_dev:io_dev",
          "@rv_tester//sysmod/null_dev:null_dev",
          "@cvm//:plusargs",
          "@cvm//:topology",
          "@cvm//:registry",
          packet + "_cc",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
