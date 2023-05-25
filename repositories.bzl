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
        #sha256 = "af87959afe497dc8dfd4c6cb66e1279cb98ccc84284619ebfec27d9c09a903de",
    )

    rules_hdl_hash="03472b2c6bf723c999c4d584359fadbeb76161fe"
    maybe(
        http_archive,
        name = "rules_hdl",
        sha256 = "3bedc45854d31a2d4b84e6f753217b23f80a8ca409c69be4ca819da820c25d0d",
        strip_prefix = "bazel_rules_hdl-{commit}".format(commit=rules_hdl_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/bazel_rules_hdl/-/archive/{commit}/bazel_rules_hdl-{commit}.tar.bz2".format(commit=rules_hdl_hash),
    )

    corearchcoverage_hash="d6a6d9bff0edf0ddc950876779a48d5cd4e10981"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="0b19c74670c0f734aa5dd5e86cca6964b853898a"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "fa12f8e459bbc3c33f65337f758c03ee7f4b63c7eda023ab80db3064e13df35e",
        strip_prefix = "cvm-{commit}".format(commit=cvm_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/{commit}/cvm-{commit}.tar.bz2".format(commit=cvm_hash),
    )

    maybe(
        http_archive,
        name = "nlohmann_json",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/nlohmann-json/-/archive/v3.11.2/nlohmann-json-v3.11.2.tar.gz",
        strip_prefix = "nlohmann-json-v3.11.2",
        sha256 = "be269122c74edb6b92371a816d7c358c00aec948219f049e8152d4c23548b1ec",
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

    whisper_hash="b2a0bdb25cb899f7b800c5d62521e401d2ed7352"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    core_arch_checker_hash="6affa8b6370842fa2422076735a0f5f0db670203"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        strip_prefix = "CoreArchChecker-{commit}".format(commit=core_arch_checker_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/{commit}/CoreArchChecker-{commit}.tar.bz2".format(commit=core_arch_checker_hash),
    )

    testgen_hash="7b65e2d3ef51d342d02c13f822722f2ecbe5f454"
    maybe(
        git_repository,
        name = "testgen",
        commit = testgen_hash,
        shallow_since = "1677278961 -0600",
        recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/testgen.git",
    )

    rules_python_version = "0.11.0"
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "94e2f4790b55823cf2a58d5e48fccf932ff879b5e868b10bd1e0fa9100ac0311",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/rules_python/-/archive/{VERSION}/rules_python-{VERSION}.tar.bz2".format(VERSION=rules_python_version)
    )

    mem_manager_hash="74b08eb05d153e792d48841f0509965ea566e9ea"
    maybe(
        http_archive,
        name = "mem_manager",
        sha256 = "ef31c29408f30b2163e96e7d7978cd9fd9226e4d569d344367a85047a7c5769e",
        strip_prefix = "mem-manager-{commit}".format(commit=mem_manager_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/mem-manager/-/archive/{commit}/mem-manager-{commit}.tar.bz2".format(commit=mem_manager_hash),
    )

    wall_clock_profiler_hash="2b1f62c002734bbd8673204fe38213cdda43219e"
    maybe(
        http_archive,
        name = "wall_clock_profiler",
        sha256 = "d897f412069f005d10cd2e339bcc2e5dfd085da99887f61e629ed9303a6185a8",
        strip_prefix = "wall_clock_profiler-{commit}".format(commit=wall_clock_profiler_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/mboisvert/wall_clock_profiler/-/archive/{commit}/wall_clock_profiler-{commit}.tar.bz2".format(commit=wall_clock_profiler_hash),
    )
