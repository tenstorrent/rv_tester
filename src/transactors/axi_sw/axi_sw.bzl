load("@rules_hdl//verilog:providers.bzl", "verilog_library")
# verilog_flist was a Tenstorrent-fork addition to rules_hdl that cvm's
# rules_hdl_compat shim doesn't ship. The _f filelist below isn't consumed by
# our verilator-only path, so the call site is gone too.

def _generate_axi_interfaces_impl(ctx):
    """Implementation of the generate_axi_interfaces rule."""

    name = ctx.attr.name
    sv_output = ctx.outputs.sv
    sw_cpp_output = ctx.outputs.sw_cpp
    mst_cpp_output = ctx.outputs.mst_cpp
    defines_output = ctx.outputs.defines

    args = ctx.actions.args()

    args.add("--topology", ctx.file.topology_json)
    args.add("--sv-output", sv_output)
    args.add("--sw-cpp-output", sw_cpp_output)
    args.add("--mst-cpp-output", mst_cpp_output)
    args.add("--defines-output", defines_output)
    if ctx.attr.axi_sw_topo:
        args.add("--axi-sw-topo", ctx.attr.axi_sw_topo)
    if ctx.attr.axi_sw_mst_topo:
        args.add("--axi-sw-mst-topo", ctx.attr.axi_sw_mst_topo)
    if ctx.attr.clk_idx_map:
        args.add("--clk-idx-map", ctx.attr.clk_idx_map)

    inputs = [
        ctx.file.topology_json,
    ]

    outputs = [sv_output, sw_cpp_output, mst_cpp_output, defines_output]

    ctx.actions.run(
        arguments = [args],
        executable = ctx.executable._gen,
        inputs = inputs,
        outputs = outputs,
    )

    return [
        DefaultInfo(
            files = depset([sv_output]),
        ),
        OutputGroupInfo(
            cpp = depset([sw_cpp_output, mst_cpp_output]),
            hdrs = depset([defines_output]),
        ),
    ]

_generate_axi_interfaces = rule(
    _generate_axi_interfaces_impl,
    attrs = {
        "topology_json": attr.label(
            mandatory = True,
            allow_single_file = [".json"],
            doc = "Topology JSON file from topology_gen (name_topology.json)",
        ),
        "sv": attr.output(
            doc = "Output SystemVerilog file with AXI interface definitions",
        ),
        "sw_cpp": attr.output(
            doc = "Output cpp file for axi_sw REGISTRY_register calls",
        ),
        "mst_cpp": attr.output(
            doc = "Output cpp file for axi_sw_mst REGISTRY_register calls",
        ),
        "defines": attr.output(
            doc = "Output C++ header with AXI loc index defines",
        ),
        "axi_sw_topo": attr.string(
            doc = "Topology path for the AXI_SW (slave) group. Overrides the generator default.",
        ),
        "axi_sw_mst_topo": attr.string(
            doc = "Topology path for the AXI_SW_MST (master) group. Overrides the generator default.",
        ),
        "clk_idx_map": attr.string(
            doc = "JSON map from DOMAIN ints to SV clock-index identifiers. Overrides the generator default.",
        ),
        "_gen": attr.label(
            default = "@rv_tester//src/transactors/axi_sw:gen_axi_interfaces",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [
        DefaultInfo,
    ],
)

def generate_axi_interfaces(name, transactions, topology, package, visibility = None, **kwargs):
    sv_output      = name + "/" + package + ".svh"
    sw_cpp_output  = name + "/" + package + "_sw_registry.cpp"
    mst_cpp_output = name + "/" + package + "_mst_registry.cpp"
    defines_output = name + "/" + package + ".h"

    _generate_axi_interfaces(
        name = name,
        sv = sv_output,
        sw_cpp = sw_cpp_output,
        mst_cpp = mst_cpp_output,
        defines = defines_output,
        topology_json = topology + ".json",
        visibility = visibility,
        **kwargs,
    )

    # The .svh output (axi_defines.svh) is a macro-define header. Plumb it
    # through verilog_library.hdrs (not srcs) so the rules_hdl_compat shim
    # adds its directory to the include search path while keeping it off the
    # Verilator command line as a top-level compile unit.
    verilog_library(
        name = name + "_sv",
        hdrs = [sv_output],
        deps = [
            "//third_party/axi:typedef",
        ],
        visibility = visibility,
    )

    native.cc_library(
        name = name + "_cc",
        srcs = [
            sw_cpp_output,
            mst_cpp_output,
        ],
        deps = [
            "@rv_tester//src/transactors/axi_sw:axi_sw",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst",
            name + "_defines",
            transactions + "_cc",
        ],
        alwayslink = True,
        visibility = visibility,
    )
    native.cc_library(
        name = name + "_defines",
        hdrs = [defines_output],
        strip_include_prefix = name,
        visibility = visibility,
    )


def axi_sw_gen(name, packet, visibility = None, cc_attrs = {}, **kwargs):

    axi_sw_dpi = name + "_dpi"
    axi_sw_sv = name + "_sv"

    verilog_library(
        name = axi_sw_sv,
        srcs = [
            "@rv_tester//src/transactors/axi_sw:axi_sw.sv"
        ],
        deps = [
            "@rv_tester//src:rv_tester_lib",
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            "@cvm//:random_sv",
            "@cvm//:registry_sv",
            packet + "_sv",
        ],
        visibility = ["//visibility:public"],
    )

    native.cc_library(
        name = axi_sw_dpi,
        srcs = [
            "@rv_tester//src/transactors/axi_sw:axi_sw.cpp",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst.cpp",
            "@rv_tester//src/transactors/axi_sw:axi.cpp",
        ],
        hdrs = [
            "@rv_tester//src/transactors/axi_sw:axi_sw.h",
            "@rv_tester//src/transactors/axi_sw:axi_sw_plusargs.h",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst.h",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst_plusargs.h",
            "@rv_tester//src/transactors/axi_sw:axi.h",
            "@rv_tester//src/transactors/axi_sw:safe_queue.h",
        ],
        deps = [
          "@rv_tester//src/common:common",
          "@rv_tester//src:plusargs",
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
