workspace(name = "rv_tester")

local_repository(
    name = "bzsim",
    path = "infra/bzsim_clone",
)

load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("@cvm//deps:repositories.bzl", "cvm_dependencies")
cvm_dependencies()

load("@cvm//deps:toolchains1.bzl", "cvm_toolchains1")
cvm_toolchains1()

load("@cvm//deps:toolchains2.bzl", "cvm_toolchains2")
cvm_toolchains2()
