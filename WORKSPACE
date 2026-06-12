workspace(name = "rv_tester")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# cvm is the parent directory (on-disk layout: cvm/rv_tester/).
local_repository(
    name = "cvm",
    path = "..",
)

# Re-use cvm's dual-emitting rules_hdl compatibility shim so downstream
# Bazel-6 consumers see the same @rules_hdl//verilog:providers.bzl%VerilogInfo
# that cvm exports.
local_repository(
    name = "rules_hdl",
    path = "../bazel/rules_hdl_compat",
)

# rules_verilog is loaded by the rules_hdl_compat shim to grab the upstream
# VerilogInfo symbol. Same pin cvm uses.
http_archive(
    name = "rules_verilog",
    url = "https://github.com/hw-bzl/bazel_rules_verilog/releases/download/v1.1.0/bazel_rules_verilog-1.1.0.tar.gz",
    sha256 = "043196310d1ba692ec217c3778663da0d232a3746ba6291d3a12d6461de24021",
    strip_prefix = "bazel_rules_verilog-1.1.0",
)

# Chain into cvm's own WORKSPACE-mode deps wiring (googletest, fmt, gflags, etc.).
load("@cvm//deps:repositories.bzl", "cvm_dependencies")
cvm_dependencies()

load("@cvm//deps:toolchains1.bzl", "cvm_toolchains1")
cvm_toolchains1()

load("@cvm//deps:toolchains2.bzl", "cvm_toolchains2")
cvm_toolchains2()

# TODO(open-source): the rest of rv_tester's transitive deps (whisper,
# mem_manager, CoreArchChecker, opensrc-axi, opensrc-axi_llc,
# opensrc-tech_cells_generic, opensrc-nlohmann-json, testgen) used to come
# from repositories.bzl / repositories2.bzl / infra/bazel/*. Until they're
# re-pointed at public mirrors, Bazel-6 WORKSPACE-mode builds of any target
# below the smoke path (rv_tester_gen, cosim, sysmod, pmu, ...) will fail to
# load.
