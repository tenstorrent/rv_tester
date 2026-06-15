"""Module extensions for rv_tester internal dependencies."""

load("//bazel:repositories.bzl", "rv_tester_repositories")
load("//infra/bazel:opensrc_repositories.bzl", "opensrc_repositories")

def _deps_impl(ctx):
    rv_tester_repositories(bzlmod = True)
    opensrc_repositories()

deps = module_extension(
    implementation = _deps_impl,
)
