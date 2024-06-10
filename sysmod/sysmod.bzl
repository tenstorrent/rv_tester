load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def sysmod_gen(name, packet, topology, visibility = None, cc_attrs = {}, **kwargs):

    sysmod_dpi = name + "_dpi"
    sysmod_sv = name + "_sv"
    name_jtag_xtor_sv = name + "jtag_xtor_sv"

    verilog_library(
		    name = name_jtag_xtor_sv,
		    srcs = ["@rv_tester//sysmod/jtag_xtor:jtag_xtor.sv"],
		    deps = [
		    "@cvm//:plusargs_sv",
		    "@cvm//:topology_sv",
		    ],
		    visibility = ["//visibility:public"],
	  )

    verilog_library(
        name = sysmod_sv,
        srcs = ["@rv_tester//sysmod:sysmod.sv"],
        deps = [
            name_jtag_xtor_sv,
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
          "@rv_tester//:plusargs",
          "@rv_tester//:structs",
          "@rv_tester//common:common",
          "@rv_tester//sysmod:device",
          "@rv_tester//sysmod/clint:clint",
          "@rv_tester//sysmod/aclint:aclint",
          "@rv_tester//sysmod/trace_cfg:trace_cfg",
          "@rv_tester//sysmod/pll_xtor:pll_xtor",
          "@rv_tester//sysmod/pm_nw_xtor:pm_nw_xtor",
          "@rv_tester//sysmod/scratchpad_xtor:scratchpad_xtor",
          "@rv_tester//sysmod/smc_xtor:smc_xtor",
          "@rv_tester//sysmod/trickbox:trickbox",
          "@rv_tester//sysmod/htif:htif",
          "@rv_tester//sysmod/uart8250:uart8250",
          "@rv_tester//sysmod/mem:mem",
          "@rv_tester//sysmod/dm:dm",
          "@rv_tester//sysmod/aplic_mmr:aplic_mmr",
          "@rv_tester//sysmod/io_dev:io_dev",
          "@rv_tester//sysmod/null_dev:null_dev",
          "@rv_tester//sysmod/heartbeat:heartbeat",
          "@rv_tester//cosim/bridge_if:bridge_if",
          "@cvm//:plusargs",
          "@cvm//:topology",
          "@cvm//:registry",
          packet + "_cc",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
