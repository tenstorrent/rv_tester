load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
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

    corearchcoverage_hash="9dd6e25498b0a566faec145aafd92c8866c5af3f"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="cd43dab0e3cad676013532280f104cb6c439c8e3"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "12fbd0835a12370676c5ddab54c22b92d4fe0fdaf06ca0c2508a81f723b484e3",
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

    whisper_hash="0990297d6c408b28869f75f7a5d40518b41d3876"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    core_arch_checker_hash="95cb0626f38a52b5ca3e1b6e37f881ac728ec410"
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

    checkin_script_hash="ddae2a510ebc0caad205e1b611d4701a4424e5ad"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )

    axi_wrapper_hash="a52e72be2790e51f2e38a78b3e134983430c7129"
    maybe(
        git_repository,
        name = "axi-wrapper",
        commit = axi_wrapper_hash,
        shallow_since = "1695754616 +0000",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/axi-wrapper.git",
    )

    axi_llc_wrapper_hash="8108092fd27a46944e192db0b78b3c4c8f38d7a3"
    maybe(
        git_repository,
        name = "axi_llc-wrapper",
        commit = axi_llc_wrapper_hash,
        shallow_since = "1695756617 +0000",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/axi_llc-wrapper.git",
    )

    common_cells_wrapper_hash="be8b4b591b8c0b6c357cfdf07339c579e8128220"
    maybe(
        git_repository,
        name = "common_cells-wrapper",
        commit = common_cells_wrapper_hash,
        shallow_since = "1695140622 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/common_cells-wrapper.git",
    )

