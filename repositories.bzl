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

    corearchcoverage_hash="c7a7036a8c3a4a9c183acf8cfacd5ab9e8e7f8f2"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="cfad64b322fb106e605ad0e6086b1d1c5e8df31d"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "5549cdad7972cb97287c212274c268412060c5fd911ea87b88a59b397165c95f",
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

    whisper_hash="ab11c10f4c1179e478c59e33307612c876f50b35"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    core_arch_checker_hash="4aba7d34459555fc9a26735b59d960842373eb0f"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        sha256 = "323215f8c0824cb118bc68c4d86efeefd273f72a841313c7a9cb949a54af5cff",
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

    mem_manager_hash="676b131ee3b6b02ec7778bc47bc58d593343b8da"
    maybe(
        http_archive,
        name = "mem_manager",
        sha256 = "d8c7b4af95af9c4aac57c4231ea8b619a64e31a2f36b9eb539b8d62f1834df68",
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

    checkin_script_hash="5c840e23253789bbeae882b804f5b4bdaa4ea0ee"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )

    opensrc_axi_llc_hash="fc54511f8cbbd24a177e1a81383c53d13882a3f8"
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
