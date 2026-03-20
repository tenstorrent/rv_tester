"""Module extensions for rv_tester internal dependencies."""

load("//infra/bazel:internal_repositories.bzl", "internal_repositories")

def _internal_deps_impl(ctx):
    internal_repositories()

internal_deps = module_extension(
    implementation = _internal_deps_impl,
)
