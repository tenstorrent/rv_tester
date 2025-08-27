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

    corearchcoverage_hash="b89e819fa9b4f55674153312caefb4098000b457"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="8efdaee83aa0ce628cca35620b7deb08f8406811"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "a232915e068e7af187b40bc4372e9b9dee62ce44b21c33dfdfeb055607b9c63c",
        strip_prefix = "cvm-{commit}".format(commit=cvm_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/{commit}/cvm-{commit}.tar.bz2".format(commit=cvm_hash),
    )

    opensrc_nlohmann_json_hash="ece38f1883dd1e59c498c63b8f53c3b4bcbc593c"
    maybe(
        git_repository,
        name = "opensrc-nlohmann-json",
        commit = opensrc_nlohmann_json_hash,
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-nlohmann-json.git",
    )

    whisper_hash="25f8b1abe8f5fa10bb6814a996fa072754714dec"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    CoreArchChecker_hash="7e9c0c182cd3f192d634c84f469269ae51516f5b"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        sha256 = "2c182d5538e710a341a8a9361f6b6fbf6f51cb2692528a3dde4622f71c5b7c61",
        strip_prefix = "CoreArchChecker-{commit}".format(commit=CoreArchChecker_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/{commit}/CoreArchChecker-{commit}.tar.bz2".format(commit=CoreArchChecker_hash),
    )

    rules_python_version = "0.11.0"
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "94e2f4790b55823cf2a58d5e48fccf932ff879b5e868b10bd1e0fa9100ac0311",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/rules_python/-/archive/{VERSION}/rules_python-{VERSION}.tar.bz2".format(VERSION=rules_python_version)
    )

    mem_manager_hash="585efffd1a79f43388339c193cdae420f32acad4"
    maybe(
        http_archive,
        name = "mem_manager",
        sha256 = "fc1d138f26405bac05f039a945fb004fc081ea49c16aa7553d9e1b0b49a4d338",
        strip_prefix = "mem-manager-{commit}".format(commit=mem_manager_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/mem-manager/-/archive/{commit}/mem-manager-{commit}.tar.bz2".format(commit=mem_manager_hash),
    )

    opensrc_wall_clock_profiler_hash="5b175bb980de9b42591b2edd0e59958ced207aef"
    maybe(
        git_repository,
        name = "opensrc-wall_clock_profiler",
        commit = opensrc_wall_clock_profiler_hash,
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-wall_clock_profiler.git",
    )

    checkin_script_hash="5a5a2da7982ad208dfc64639bfc90f6f454946f7"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )

    aegissocspec_hash="d116a4c3544f1190bed271da71ec3883b95f84bd"
    git_repository(
        name = "aegissocspec",
        commit = aegissocspec_hash,
        recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:specifications/arch-export-controlled/aegissocspec.git",
    )

    risc_v_cpu_spec_hash="3a0cd65fab8dd55c77d482323c91fbe35f069edd"
    maybe(
        git_repository,
        name = "risc-v-cpu-spec",
        commit = risc_v_cpu_spec_hash,
        shallow_since = "1664916258 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:specifications/arch-export-controlled/risc-v-cpu-spec.git",
    )
