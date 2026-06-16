workspace(name = "rv_tester")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# cvm + its bundled rules_hdl_compat shim come from aus-gitlab. Pinned to
# the same commit MODULE.bazel uses. Lets rv_tester check out anywhere on
# disk without an adjacent cvm/.
git_repository(
    name = "cvm",
    remote = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm.git",
    commit = "438b90fdfa6c8449e124e756f0ddc392cf2fe93e",
)

http_archive(
    name = "rules_hdl",
    urls = ["https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/438b90fdfa6c8449e124e756f0ddc392cf2fe93e/cvm-438b90fdfa6c8449e124e756f0ddc392cf2fe93e.tar.bz2"],
    strip_prefix = "cvm-438b90fdfa6c8449e124e756f0ddc392cf2fe93e/bazel/rules_hdl_compat",
    sha256 = "7e1a0f6e137936634e68b3fc5327b58e0c0823295275c2ba97ee9f65aefcc3d4",
)

# rules_verilog is loaded by the rules_hdl_compat shim to grab the upstream
# VerilogInfo symbol. Same pin cvm uses.
http_archive(
    name = "rules_verilog",
    url = "https://github.com/hw-bzl/bazel_rules_verilog/releases/download/v1.1.0/bazel_rules_verilog-1.1.0.tar.gz",
    sha256 = "043196310d1ba692ec217c3778663da0d232a3746ba6291d3a12d6461de24021",
    strip_prefix = "bazel_rules_verilog-1.1.0",
)

# Shared with bzlmod via //bazel:external_deps_ext.bzl. Declares the
# aus-gitlab repos (CoreArchChecker, mem_manager, opensrc-nlohmann-json)
# so Bazel-6 WORKSPACE-mode resolves the same set bzlmod does.
load("//bazel:external_deps.bzl", "rv_tester_external_deps")
rv_tester_external_deps()

# @whisper has its own MODULE.bazel + deps.bzl. Bzlmod takes it via
# bazel_dep + git_override in MODULE.bazel (which processes the
# MODULE.bazel and pulls @pybind11_bazel + boost.* from BCR); WORKSPACE
# declares it here directly and chains whisper_dependencies() to pull
# @pybind11_bazel + @pybind11 from github via http_archive.
git_repository(
    name = "whisper",
    commit = "c349731df9bab5281d74ce862aebfcd72cd85f9e",
    shallow_since = "1656867071 -0400",
    remote = "https://aus-gitlab.local.tenstorrent.com/riscv/swerv-iss.git",
)
load("@whisper//:deps.bzl", "whisper_dependencies")
whisper_dependencies()

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
