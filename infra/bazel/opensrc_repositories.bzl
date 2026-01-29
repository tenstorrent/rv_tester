load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

# These should only be used within rv_tester

def opensrc_repositories():

    opensrc_common_verification_hash="6ce78351835a392ab06ffb0fabacb7fb5263de0a"
    maybe(
        git_repository,
        name = "opensrc-common_verification",
        commit = opensrc_common_verification_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_verification.git",
    )

    opensrc_tech_cells_generic_hash="d4818a737c91e6bb6765047a3a7fde19a8964c05"
    maybe(
        git_repository,
        name = "opensrc-tech_cells_generic",
        commit = opensrc_tech_cells_generic_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-tech_cells_generic.git",
    )

    opensrc_common_cells_hash="8c522859dd3b996c9b6d5fca4906b9fe6987006c"
    maybe(
        git_repository,
        name = "opensrc-common_cells",
        commit = opensrc_common_cells_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_cells.git",
    )

    opensrc_axi_hash="7387de678e025b809341fa8a2893ef5e931e6d4d"
    maybe(
        git_repository,
        name = "opensrc-axi",
        commit = opensrc_axi_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi.git",
    )

    opensrc_register_interface_hash="d47c3c2cf839a84a6280f9ca1033b80f102933a0"
    maybe(
        git_repository,
        name = "opensrc-register_interface",
        commit = opensrc_register_interface_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-register_interface.git",
    )

    opensrc_apb_hash="211dd56c5f9ef0fc1385146eb5ffcb9b51868180"
    maybe(
        git_repository,
        name = "opensrc-apb",
        commit = opensrc_apb_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-apb.git",
    )

    opensrc_axi_llc_hash="8d6f037eeb26d141d00a9f450b33f34c9bb4abde"
    maybe(
        git_repository,
        name = "opensrc-axi_llc",
        commit = opensrc_axi_llc_hash,
        shallow_since = "1695756617 +0000",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi_llc.git",
    )

    tensix_tt_tech_hash="8b250cc57664707fc112514eb61a8d46cb78b61b"
    maybe(
        git_repository,
        name = "tensix-tt_tech",
        commit = tensix_tt_tech_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/tensix-forks/tensix-tt_tech.git",
    )

    tensix_hw_common_old_hash="4030f2ec9d9d3a0e3cc850051f336b9712138e18"
    maybe(
        git_repository,
        name = "tensix-hw-common-old",
        commit = tensix_hw_common_old_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/tensix-forks/tensix-hw-common-old.git",
    )
