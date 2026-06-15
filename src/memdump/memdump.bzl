def memdump_gen(name, packet, visibility = None, cc_attrs = {}, **kwargs):

    memdump_dpi = name + "_dpi"

    native.cc_library(
        name = memdump_dpi,
        srcs = [
            "@rv_tester//src/memdump:memdump.cpp",
        ],
        hdrs = [
            "@rv_tester//src/memdump:memdump.h",
            "@rv_tester//src/sysmod:sysmod_rpc.h",
            "@rv_tester//src/transactors/axi_sw:axi.h",
            "@rv_tester//src/transactors/axi_sw:safe_queue.h",
        ],
        deps = [
            "@rv_tester//src/sysmod:sysmod_plusargs",
            "@rv_tester//src/sysmod/htif:htif",
            "@rv_tester//src/transactors/axi_sw:axi_sw_mst",
            "@rv_tester//src/common:transactor",
            "@rv_tester//src/common:common",
            "@rv_tester//src:structs",
            packet + "_cc",
            "@cvm//:plusargs",
            "@cvm//:logger",
            "@cvm//:registry",
            "@cvm//:topology",
            "@cvm//:messenger",
        ],
        alwayslink = True,
        visibility = visibility,
    )
