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

    opensrc_tech_cells_generic_hash="601da41b77f31ffb0fd6fc9259e99fe558d18de2"
    maybe(
        git_repository,
        name = "opensrc-tech_cells_generic",
        commit = opensrc_tech_cells_generic_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-tech_cells_generic.git",
    )

    opensrc_common_cells_hash="16135359d85eba15d918998748fd67856cd9f70d"
    maybe(
        git_repository,
        name = "opensrc-common_cells",
        commit = opensrc_common_cells_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:opensrc/opensrc-common_cells.git",
    )

    opensrc_axi_hash="d8d3406f851700b68a3f4e57806a445c3b6415ee"
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

