load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def overlay_driver_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    overlay_driver_dpi = name + "_dpi"
    overlay_driver_sv = name + "_sv"

    native.cc_library(
        name = overlay_driver_dpi,
        srcs = [
            "@rv_tester//overlay_driver:overlay_driver.cpp",
            "@rv_tester//overlay_driver:scratchpad_random_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//transactors/axi_sw:safe_queue.h",
            "@rv_tester//transactors/axi_sw:axi.h",
            "@rv_tester//overlay_driver:overlay_driver.hpp",
            "@rv_tester//overlay_driver:scratchpad_random_sequence.hpp",
        ],
        deps = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi_plusargs",
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//transactors/axi_sw:axi_sw_mst",
            "@rv_tester//common:transactor",
            "@rv_tester//:structs",
            "@rv_tester//sysmod:device",
            "@rv_tester//cosim/whisper_if:whisper_if",
            "@mem_manager//:mem_manager",
            "@rv_tester//common:common",
            "@cvm//:bitmanip",
            "@cvm//:topology",
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
        name = overlay_driver_sv,
        srcs = ["@rv_tester//overlay_driver:overlay_driver.sv"],
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:random_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
        ],
        visibility = visibility,
    )
