load("@corearchcoverage//infra/bazel:repositories.bzl","corearchcoverage_dependencies")

def _deps_impl(ctx):
    corearchcoverage_dependencies()

deps2 = module_extension(
    implementation = _deps_impl,
)
