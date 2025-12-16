workspace(name = "rv_tester")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

local_repository(
    name = "bzsim",
    path = "infra/bzsim_clone",
)

load("@bzsim//:repositories.bzl", "bzsim_dependencies")
bzsim_dependencies()

load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

load("//infra/bazel:opensrc_repositories.bzl", "opensrc_repositories")
opensrc_repositories()

load("@whisper//:deps.bzl", "whisper_dependencies")
whisper_dependencies()

load("//:repositories2.bzl", "rv_tester_repositories2")
rv_tester_repositories2()

load("//infra/bazel:dependencies.bzl", "rv_tester_dependencies")
rv_tester_dependencies()

load("//infra/bazel:dependencies2.bzl", "rv_tester_dependencies2")
rv_tester_dependencies2()

load("//infra/bazel:dependencies3.bzl", "rv_tester_dependencies3")
rv_tester_dependencies3()

# don't want testgen in repositories.bzl, as it's only for internal use
# let other repos' testgen supercede this one in downstream repos
testgen_hash="50e12d558b97250deb7a671a524278a61a06a60c"
git_repository(
    name = "testgen",
    commit = testgen_hash,
    shallow_since = "1677278961 -0600",
    recursive_init_submodules = True,
    remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/testgen.git",
)

load("@testgen//:repositories.bzl", "testgen_dependencies")
testgen_dependencies()

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("@cvm//deps:repositories.bzl", "cvm_dependencies")
cvm_dependencies()

load("@cvm//deps:toolchains1.bzl", "cvm_toolchains1")
cvm_toolchains1()

load("@cvm//deps:toolchains2.bzl", "cvm_toolchains2")
cvm_toolchains2()


load(
    "@rules_vcs//vcs:repositories.bzl",
    "rules_vcs_dependencies",
)
rules_vcs_dependencies()

load(
    "@rules_verilator//verilator:repositories.bzl",
    "rules_verilator_dependencies",
    "rules_verilator_toolchains"
)
rules_verilator_dependencies()
rules_verilator_toolchains()

load("@rules_m4//m4:m4.bzl", "m4_register_toolchains")
m4_register_toolchains(extra_copts=["-Wno-error=sign-compare", "-Wno-error=unused-parameter"])

load("@rules_flex//flex:flex.bzl", "flex_register_toolchains")
flex_register_toolchains(extra_copts=["-Wno-error=misleading-indentation", "-Wno-error=pointer-sign"])

load("@rules_bison//bison:bison.bzl", "bison_register_toolchains")
bison_register_toolchains(extra_copts=["-Wno-error=misleading-indentation", "-Wno-error=sign-compare" , "-Wno-error=unused-parameter", "-Wno-error=unused-but-set-variable"])

load("//infra/bazel:testlist.bzl", "load_testlists")
load_testlists()
