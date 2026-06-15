load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("@rv_tester//src/pmu:pmu_fragment_gen.bzl", "pmu_fragment_gen")

def pmu_gen(
    name,
    packet,
    topology,
    harness,
    pmu_spec = "@rv_tester//src/pmu:pmu_spec",
    pmu_fragments = None,
    visibility = None,
    cc_attrs = {},
    **kwargs
):

    pmu_dpi = name + "_dpi"
    pmu_sv = name + "_sv"

    # Use provided pmu_fragments or generate new ones
    if pmu_fragments:
        fragments_name = pmu_fragments
    else:
        fragments_name = name + "_pmu_fragments"
        pmu_fragment_gen(
            name = fragments_name,
            pmu_spec = pmu_spec,
            pmu_template = "@rv_tester//src/pmu:pmu.sv",
            cc_attrs = cc_attrs,
        )

    # Merge cc_attrs with alwayslink, giving precedence to cc_attrs if it contains alwayslink
    cc_library_attrs = dict(cc_attrs)
    if "alwayslink" not in cc_library_attrs:
        cc_library_attrs["alwayslink"] = True

    native.cc_library(
        name = pmu_dpi,
        srcs = [
            "@rv_tester//src/pmu:pmu.cpp"
        ],
        hdrs = [
            "@rv_tester//src/pmu:pmu.hpp",
        ],
        deps = [
            "@rv_tester//src:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:logger",
            "@cvm//:bitmanip",
            "@cvm//:registry",
            "@rv_tester//src/sysmod:sysmod_plusargs",
            fragments_name + "_cc",  # Add generated headers
         ],
        visibility = visibility,
        **cc_library_attrs,
    )

    # Use generated pmu.sv (with inlined code) from pmu_fragments_sv
    verilog_library(
        name = pmu_sv,
        deps = [
            "@cvm//:plusargs_sv",
            "@cvm//:topology_sv",
            packet + "_sv",
            topology + "_sv",
            harness,
            fragments_name + "_sv",  # Contains generated pmu.sv with inlined code
        ],
        visibility = visibility,
    )
