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

    cosim_hash="1eedf35dec879eef3eca59c9ac2731b8252de727"
    maybe(
        git_repository,
          name = "cosim",
          commit = cosim_hash,
          remote = "git@aus-gitlab.local.tenstorrent.com:manees/cosim.git",
    )

    cvm_hash="9ff5b52b08894b99cd834b9568ceca333f7ca07c"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "9df0ed0929b35614bd5e75a5c91f2eddbc02cfa8b20a3ed6e4126abc4d265004",
        strip_prefix = "cvm-{commit}".format(commit=cvm_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/{commit}/cvm-{commit}.tar.bz2".format(commit=cvm_hash),
    )

    maybe(
        http_archive,
        name = "nlohmann_json",
        url = "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz",
        strip_prefix = "json-3.11.2",
        sha256 = "d69f9deb6a75e2580465c6c4c5111b89c4dc2fa94e3a85fcd2ffcd9a143d9273",
        build_file_content = """
cc_library(
    name = "json",
    hdrs = glob(["single_include/nlohmann/json.hpp"]),
    strip_include_prefix = "single_include",
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
)
    """
    )

