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

    corearchcoverage_hash="a3026acb543fc2a58ec3ca9e525b7b6f7761a6b9"
    maybe(
        git_repository,
        name = "corearchcoverage",
        commit = corearchcoverage_hash,
        #recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/corearchcoverage.git",
    )

    cvm_hash="bae84efa3d5248c1a5765c87b7520a200015f9db"
    maybe(
        http_archive,
        name = "cvm",
        sha256 = "153d80c77e09de39ed91f3d60fa257df24497d008f6c63c3378091f95179ddbe",
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

    whisper_hash="d654b2688fa5757a8c1e567908cc1ae524fe6e91"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    CoreArchChecker_hash="d0b4f6c60815966a4ef182fb623caba2132cfca9"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        sha256 = "2a556f3ccc458b67cff523076f7f7dc1fc07dede85fe04e5199a2bb5fe467bb8",
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

    mem_manager_hash="937feeda4906c3ac6947bb4deb2dd2df75cf765e"
    maybe(
        http_archive,
        name = "mem_manager",
        sha256 = "6bfefda857cf12650432e379246778f4411d13b2015ae1b4ec3aa3a2c9003395",
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

    checkin_script_hash="46b6f57e3c49fb612b53feabf283738b7c87fbf9"
    maybe(
        git_repository,
        name = "checkin-script",
        commit = checkin_script_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv_global/checkin-script.git",
    )
