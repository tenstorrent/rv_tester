load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def internal_repositories():
    testgen_hash="50e12d558b97250deb7a671a524278a61a06a60c"
    git_repository(
        name = "testgen",
        commit = testgen_hash,
        shallow_since = "1677278961 -0600",
        recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/testgen.git",
    )

