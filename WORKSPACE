workspace(name = "rv_tester")

local_repository(
    name = "bzsim",
    path = "infra/bzsim_clone",
)

load("@bzsim//:repositories.bzl", "bzsim_dependencies")
bzsim_dependencies()

load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

load("@cva6-wrapper//:defs.bzl", "cva6_wrapper_repositories")
cva6_wrapper_repositories()

load("@risc-p-cores//infra/bazel:repositories.bzl", "risc_p_cores_dependencies")
risc_p_cores_dependencies()

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("@cvm//deps:repositories.bzl", "cvm_dependencies")
cvm_dependencies()

load("@cvm//deps:toolchains1.bzl", "cvm_toolchains1")
cvm_toolchains1()

load("@cvm//deps:toolchains2.bzl", "cvm_toolchains2")
cvm_toolchains2()

load("@risc-p-cores//infra/bazel:repositories.bzl", "risc_p_cores_dependencies")
risc_p_cores_dependencies()

load("@rv-common//infra/bazel:repositories.bzl", "rv_common_dependencies")
rv_common_dependencies()

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

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")
# This sets up some common toolchains for building targets. For more details, please see
# https://bazelbuild.github.io/rules_foreign_cc/0.8.0/flatten.html#rules_foreign_cc_dependencies
rules_foreign_cc_dependencies()

load("@chisel-math//:repositories.bzl", "chisel_math_dependencies")
chisel_math_dependencies()

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()

load("@rules_chisel//:repositories.bzl", "rules_chisel_dependencies")
rules_chisel_dependencies()

load("@rules_chisel//:toolchains1.bzl", "rules_chisel_toolchains1")
rules_chisel_toolchains1()

load("@rules_chisel//:toolchains2.bzl", "rules_chisel_toolchains2")
rules_chisel_toolchains2()

load("@rules_chisel//:toolchains3.bzl", "rules_chisel_toolchains3")
rules_chisel_toolchains3()

load("//infra/bazel:testlist.bzl", "load_testlists")
load_testlists()
