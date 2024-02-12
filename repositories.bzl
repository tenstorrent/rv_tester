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

    corearchcoverage_hash="4447b1360777bcf0c8a7b0e9d7e5d19058fa0513"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="b923f35d29f44cda13776c0539aa5dc91807e18f"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "b5dd5d9de9104aa7ac9cafa7586d5b891096a3f67bd3dad204f37fc64c1cb7a9",
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

    whisper_hash="76948a3ab95afa22ffeb6e570aecdf9a5330afdb"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    core_arch_checker_hash="18aa107d1021bbcc668efaae86721e2d15c84e5b"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        sha256 = "7c4942ca94553faad728ec7a408abab7d850a9476fc63ac03a20528330120604",
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

    opensrc_wall_clock_profiler_hash="5b175bb980de9b42591b2edd0e59958ced207aef"
    maybe(
        git_repository,
        name = "opensrc-wall_clock_profiler",
        commit = opensrc_wall_clock_profiler_hash,
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-wall_clock_profiler.git",
    )

    checkin_script_hash="c8b7b1ac3c7f3597969789e5a76bdafdd9386f6f"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )

    opensrc_axi_llc_hash="93bdcc2288689aed78d673cace583d30a93e59b7"
    maybe(
        git_repository,
        name = "opensrc-axi_llc",
        commit = opensrc_axi_llc_hash,
        shallow_since = "1695756617 +0000",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi_llc.git",
    )

    aplic_model_hash="b5cd6bc4f2ec2a9fff32e68a670b23d9b777f2c1"
    git_repository(
        name = "aplic",
        commit = aplic_model_hash,
        recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/aplic.git",
    )
