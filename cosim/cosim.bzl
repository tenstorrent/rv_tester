load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")

def cosim_gen(name, topology, visibility = None, cc_attrs = {}, **kwargs):

    cosim_dpi = name + "_dpi"
    cosim_sv = name + "_sv"

    packet_gen(
        name = name + "_transactions",
        src = "@rv_tester//cosim/transactions:transactions.yml",
        package = "cosim_transactions",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    native.cc_library(
        name = name + "_rvfi",
        srcs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.cpp",
            "@rv_tester//cosim/utils/eot:eot.cpp",
        ],
        hdrs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.h",
            "@rv_tester//cosim/utils/eot:eot.h",
        ],
        deps = [
                name + "_transactions_cc",
                "@rv_tester//sysmod/htif:htif",
                "@rv_tester//cosim/bridge_if:bridge_if",
                "@rv_tester//cosim/bridge:bridge",
                "@rv_tester//cosim/dut_if/rvfi:rvfi_extern",
                "@rv_tester//cosim/utils/bot:bot",
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
                "@rv_tester//:defines",
                "@cvm//:plusargs_sv",
                "@cvm//:topology_sv",
                name + "_transactions_sv",
               ],
        visibility = visibility,
    )

    native.cc_library(
        name = cosim_dpi,
        deps = [
                name + "_rvfi",
                "@cvm//:plusargs",
                name + "_transactions_cc"
               ],
        alwayslink = True,
        visibility = visibility,
    )
