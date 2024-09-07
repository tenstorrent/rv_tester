load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

# These should only be used within rv_tester

def opensrc_repositories():

    opensrc_common_verification_hash="55db000bd24eae3091507ee2a59ce2f2360dee3e"
    maybe(
        git_repository,
        name = "opensrc-common_verification",
        commit = opensrc_common_verification_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_verification.git",
    )

    opensrc_tech_cells_generic_hash="6aac0ff239ce061e9023dc5ce8368025805721b0"
    maybe(
        git_repository,
        name = "opensrc-tech_cells_generic",
        commit = opensrc_tech_cells_generic_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-tech_cells_generic.git",
    )

    opensrc_common_cells_hash="11d83b1b94bc2b25efe6630e9da7c7e1fcb579a3"
    maybe(
        git_repository,
        name = "opensrc-common_cells",
        commit = opensrc_common_cells_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_cells.git",
    )

    opensrc_axi_hash="b9bd1aaa35e402f56bb9a579da835eef83db83c2"
    maybe(
        git_repository,
        name = "opensrc-axi",
        commit = opensrc_axi_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-axi.git",
    )

    opensrc_register_interface_hash="1a67c3b018bb6bda6e93872cd2e418cf0c9865cc"
    maybe(
        git_repository,
        name = "opensrc-register_interface",
        commit = opensrc_register_interface_hash,
        shallow_since = "1669784673 -0600",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-register_interface.git",
    )

    opensrc_apb_hash="07457725498babfacd5736418d6445107453d790"
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
