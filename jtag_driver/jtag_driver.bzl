load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def jtag_driver_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):

    jtag_driver_dpi = name + "_dpi"
    jtag_driver_sv = name + "_sv"

    native.cc_library(
        name = jtag_driver_dpi,
        srcs = [
            "@rv_tester//jtag_driver:jtag_driver.cpp",
            "@rv_tester//jtag_driver:jtag_socket_sequence.cpp",
        ],
        hdrs = [
            "@rv_tester//jtag_driver:jtag_driver.hpp",
            "@rv_tester//jtag_driver:jtag_socket_sequence.hpp",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//common:transactor",
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
        name = jtag_driver_sv,
        srcs = ["@rv_tester//jtag_driver:jtag_driver.sv"],
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
