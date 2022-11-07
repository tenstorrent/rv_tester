load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def rv_tester_repositories():

    maybe(
        http_archive,
        name = "bazel_skylib",
        urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.2.0/bazel-skylib-1.2.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.0/bazel-skylib-1.2.0.tar.gz",
        ],
        sha256 = "af87959afe497dc8dfd4c6cb66e1279cb98ccc84284619ebfec27d9c09a903de",
    )

    rules_hdl_hash="03472b2c6bf723c999c4d584359fadbeb76161fe"
    maybe(
        http_archive,
        name = "rules_hdl",
        sha256 = "3bedc45854d31a2d4b84e6f753217b23f80a8ca409c69be4ca819da820c25d0d",
        strip_prefix = "bazel_rules_hdl-{commit}".format(commit=rules_hdl_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/bazel_rules_hdl/-/archive/{commit}/bazel_rules_hdl-{commit}.tar.bz2".format(commit=rules_hdl_hash),
    )

    axi_sw_hash="6ef0d94a09533338a4cc8088aa702ae7ad4fc75a"
    maybe(
        http_archive,
        name = "axi-sw",
        sha256 = "a3bde395fdf47897efc6c47fd6ca07073d06109f09ad5910d644342bab0d559d",
        strip_prefix = "axi-sw-{commit}".format(commit=axi_sw_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/axi-sw/-/archive/{commit}/axi-sw-{commit}.tar.bz2".format(commit=axi_sw_hash),
    )
