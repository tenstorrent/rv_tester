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

# Fail-on-call pybind11_bazel stub so @whisper's BUILD load resolves under
# WORKSPACE-mode (mirrors the bzlmod local_path_override in MODULE.bazel).
local_repository(
    name = "pybind11_bazel",
    path = "third_party/pybind11_bazel_stub",
)

# Shared with bzlmod via //bazel:external_deps_ext.bzl. Declares the
# aus-gitlab repos (CoreArchChecker, mem_manager, whisper,
# opensrc-nlohmann-json) so Bazel-6 WORKSPACE-mode resolves the same set
# bzlmod does.
load("//bazel:external_deps.bzl", "rv_tester_external_deps")
rv_tester_external_deps()

# Chain into cvm's own WORKSPACE-mode deps wiring (googletest, fmt, gflags, etc.).
load("@cvm//deps:repositories.bzl", "cvm_dependencies")
cvm_dependencies()

load("@cvm//deps:toolchains1.bzl", "cvm_toolchains1")
cvm_toolchains1()

load("@cvm//deps:toolchains2.bzl", "cvm_toolchains2")
cvm_toolchains2()

# Wire up @rv_tester_pypi via pip_parse so axi_sw's gen_axi_interfaces
# py_binary (and any other py_binary that uses pyyaml) resolves the same
# package set the bzlmod pip.parse declares in MODULE.bazel.
load("@rules_python//python:pip.bzl", "pip_parse")
pip_parse(
    name = "rv_tester_pypi",
    requirements_lock = "//bazel:requirements.txt",
)
load("@rv_tester_pypi//:requirements.bzl", "install_deps")
install_deps()
