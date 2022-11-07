load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

rules_hdl_hash="03472b2c6bf723c999c4d584359fadbeb76161fe"
maybe(
    http_archive,
    name = "rules_hdl",
    sha256 = "3bedc45854d31a2d4b84e6f753217b23f80a8ca409c69be4ca819da820c25d0d",
    strip_prefix = "bazel_rules_hdl-{commit}".format(commit=rules_hdl_hash),
    url = "https://aus-gitlab.local.tenstorrent.com/riscv/bazel_rules_hdl/-/archive/{commit}/bazel_rules_hdl-{commit}.tar.bz2".format(commit=rules_hdl_hash),
)
