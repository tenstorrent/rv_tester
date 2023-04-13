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
    
    core_arch_coverage_hash="7ca1c102bc0a148a1185b27cc1e30594638e6e84"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = core_arch_coverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="6fd54a5c716366b2f2a63fe925c3b23bf59146b8"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "b2da7a598b69d80a7542beaf3e4529f8eb2375019ce91da304913dbd60043f1a",
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

    whisper_hash="1539a0653675e2591dd61cd0991003de5b12d742"
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

    testgen_hash="9249e380eb48050e8fd3c976044da5c27ae68289"
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

    cva6_wrapper_hash="9467108f4fd2ff61544df8d2aaadf4df250a0195"
    maybe(
        http_archive,
        name = "cva6-wrapper",
        sha256 = "b7fd6700cf8bcbc6b8c4a8d05a75befc407086c699d3e3537eab87dc28548c5c",
        strip_prefix = "cva6-wrapper-{commit}".format(commit=cva6_wrapper_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cva6-wrapper/-/archive/{commit}/cva6-wrapper-{commit}.tar.bz2".format(commit=cva6_wrapper_hash),
    )

    risc_p_cores_hash="592b3c5272218d45212ded44c74e86fd2fa09b84"
    git_repository(
        name = "risc-p-cores",
        commit = risc_p_cores_hash,
        shallow_since = "1659122452 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/risc-p-cores.git",
    )