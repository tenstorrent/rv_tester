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

    corearchcoverage_hash="731a375d84285fc327d76833edce20549fcb48d7"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="8bbdb883c54ecdc34fbd87afe059def38e263afc"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "a33447dfd9a04c0f1334e31af35539ac9f1ff6d90aeeb4b82179ccfaef0aadaf",
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

    whisper_hash="cbea81c583e3305637e5ae04867e8252c6c08ca0"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    core_arch_checker_hash="93f929809c47bfa07dcee719249ea6152e59eee2"
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

    checkin_script_hash="460dbaf8d380fc0375cf07b1dcb29f0f63971421"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )
