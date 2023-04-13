workspace(name = "rv_tester")

local_repository(
    name = "bzsim",
    path = "infra/bzsim_clone",
)

load("@bzsim//:repositories.bzl", "bzsim_dependencies")
bzsim_dependencies()

load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

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
m4_register_toolchains()

load("@rules_flex//flex:flex.bzl", "flex_register_toolchains")
flex_register_toolchains()

load("@rules_bison//bison:bison.bzl", "bison_register_toolchains")
bison_register_toolchains()

load("//infra/bazel:testlist.bzl", "load_testlists")
load_testlists()
