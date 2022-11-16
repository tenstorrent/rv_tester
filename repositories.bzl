load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
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

    axi_sw_hash="56215c19e85065cf81e71b0732b35ab51c27167f"
    maybe(
        http_archive,
        name = "axi-sw",
        sha256 = "c8e5f01446dbb719deaa376f48acfd6d74de5bb0ee129372490e630e85cb2f92",
        strip_prefix = "axi-sw-{commit}".format(commit=axi_sw_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/axi-sw/-/archive/{commit}/axi-sw-{commit}.tar.bz2".format(commit=axi_sw_hash),
    )

    cosim_hash="70371df590c527a9194e15ff5e15c6a8f360f027"
    maybe(
        git_repository,
          name = "cosim",
          commit = cosim_hash,
          remote = "git@aus-gitlab.local.tenstorrent.com:manees/cosim.git",
    )
