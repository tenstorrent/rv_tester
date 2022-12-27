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

    cvm_hash="522dcf50eb3e23b771d1d5491d16646efa008539"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "0457381ac471ef4c1ee362dcd39291d9aa686afe75b9350758c9d249676d7aa4",
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
    strip_include_prefix = "single_include/",
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
)
    """
    )

    whisper_hash="4512d60f4b53f7a71d082e7cdba58b765262e2ed"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )
    
    core_arch_checker_hash="b944577c128e2c09dda8db43555a84f599280a82"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        strip_prefix = "CoreArchChecker-{commit}".format(commit=core_arch_checker_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/{commit}/CoreArchChecker-{commit}.tar.bz2".format(commit=core_arch_checker_hash),
    )

    rules_python_version = "0.11.0"
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "1fe4f7f532a7af16bbe157a7757d7550c23f64798be07638f1f2df521bcf0d3c",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://github.com/bazelbuild/rules_python/archive/{}.zip".format(rules_python_version),
    )
