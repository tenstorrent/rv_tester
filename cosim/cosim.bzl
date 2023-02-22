load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@cvm//:defs.bzl", "packet_gen")

def cosim_gen(name, topology, visibility = None, cc_attrs = {}, **kwargs):

    prefix = "cosim"
    cosim_dpi = name + "_dpi"
    cosim_sv = name + "_sv"

    packet_gen(
        name = prefix + "_transactions",
        src = "@rv_tester//cosim/transactions:transactions.yml",
        topology = topology,
        cc_attrs = cc_attrs,
    )

    native.cc_library(
        name = prefix + "_rvfi",
        srcs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.cpp",
            "@rv_tester//cosim/utils/eot:eot.cpp",
        ],
        hdrs = [
            "@rv_tester//cosim/dut_if/rvfi:rvfi.h",
            "@rv_tester//cosim/utils/eot:eot.h",
        ],
        deps = [
                "cosim_transactions_cc",
                "@rv_tester//sysmod/htif:htif",
                "@rv_tester//cosim/bridge_if:bridge_if",
                "@rv_tester//cosim/bridge:bridge",
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
                "cosim_transactions_sv",
               ],
        visibility = visibility,
    )

    native.cc_library(
        name = cosim_dpi,
        deps = [
                prefix + "_rvfi",
                "@cvm//:plusargs",
                "cosim_transactions_cc"
               ],
        alwayslink = True,
        visibility = visibility,
    )
