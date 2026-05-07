def memdump_gen(name, packet, visibility = None, cc_attrs = {}, **kwargs):

    memdump_dpi = name + "_dpi"

    native.cc_library(
        name = memdump_dpi,
        srcs = [
            "@rv_tester//memdump:memdump.cpp",
        ],
        hdrs = [
            "@rv_tester//memdump:memdump.h",
            "@rv_tester//sysmod:sysmod_rpc.h",
            "@rv_tester//transactors/axi_sw:axi.h",
            "@rv_tester//transactors/axi_sw:safe_queue.h",
        ],
        deps = [
            "@rv_tester//sysmod:sysmod_plusargs",
            "@rv_tester//sysmod/htif:htif",
            "@rv_tester//transactors/axi_sw:axi_sw_mst",
            "@rv_tester//common:transactor",
            "@rv_tester//common:common",
            "@rv_tester//:structs",
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
