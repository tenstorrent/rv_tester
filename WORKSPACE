workspace(name = "rv_tester")

# Drive our own WORKSPACE through the same macros downstream consumers use,
# so the API and our internal use stay in sync. Two `dependencies*` stages
# are required because Starlark only allows `load()` at file scope:
# stage 2's top-level load of `@rv_tester_pypi//:requirements.bzl` consumes
# the repo declared by stage 1's `pip_parse()` body call.
load("//bazel:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

load("//infra/bazel:dependencies.bzl", "rv_tester_dependencies")
rv_tester_dependencies()

load("//infra/bazel:dependencies2.bzl", "rv_tester_dependencies2")
rv_tester_dependencies2()
