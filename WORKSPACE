workspace(name = "rv_tester")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

local_repository(
    name = "bzsim",
    path = "infra/bzsim_clone",
)

load("@bzsim//:repositories.bzl", "bzsim_dependencies")
bzsim_dependencies()

opensrc_common_verification_hash="55db000bd24eae3091507ee2a59ce2f2360dee3e"
maybe(
    git_repository,
    name = "opensrc-common_verification",
    commit = opensrc_common_verification_hash,
    shallow_since = "1658763133 -0500",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_verification.git",
)

opensrc_tech_cells_generic_hash="601da41b77f31ffb0fd6fc9259e99fe558d18de2"
maybe(
    git_repository,
    name = "opensrc-tech_cells_generic",
    commit = opensrc_tech_cells_generic_hash,
    shallow_since = "1658763133 -0500",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-tech_cells_generic.git",
)

opensrc_common_cells_hash="7abf231af21e3b70de33bce1c53e093c1c775d7b"
maybe(
    git_repository,
    name = "opensrc-common_cells",
    commit = opensrc_common_cells_hash,
    shallow_since = "1658763133 -0500",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_cells.git",
)

opensrc_axi_hash="95166f2a48f86af247b9fecd0b1b8ed1cd689d3c"
maybe(
    git_repository,
    name = "opensrc-axi",
    commit = opensrc_axi_hash,
    shallow_since = "1669784673 -0600",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi.git",
)

opensrc_register_interface_hash="69a1114e40fc58fcf9cf9af0cdb8bb4cadab3d84"
maybe(
    git_repository,
    name = "opensrc-register_interface",
    commit = opensrc_register_interface_hash,
    shallow_since = "1669784673 -0600",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-register_interface.git",
)

opensrc_apb_hash="d2fc024b3337f422b9f56186ddc1060cda26d22e"
maybe(
    git_repository,
    name = "opensrc-apb",
    commit = opensrc_apb_hash,
    shallow_since = "1669784673 -0600",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-apb.git",
)

opensrc_axi_llc_hash="6783a802e257840458e72d702e0f68f90170ebfc"
maybe(
    git_repository,
    name = "opensrc-axi_llc",
    commit = opensrc_axi_llc_hash,
    shallow_since = "1695756617 +0000",
    remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi_llc.git",
)

load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

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

load("@corearchcoverage//infra/bazel:repositories.bzl","corearchcoverage_dependencies")
corearchcoverage_dependencies()

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
