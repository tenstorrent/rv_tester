"""Shared declaration of rv_tester's external aus-gitlab dependencies.

Holds repos that DON'T have their own MODULE.bazel (no bzlmod-native
dep wiring). Called from two places:
- `MODULE.bazel` via the module extension in `:external_deps_ext.bzl` so
  bzlmod resolves the repos below (and `use_repo(...)` exposes each name).
- `WORKSPACE` via `load("//bazel:external_deps.bzl",
  "rv_tester_external_deps") + rv_tester_external_deps()` so Bazel-6
  WORKSPACE-mode resolves the same set.

@whisper is NOT here — it has its own MODULE.bazel so bzlmod takes it via
`bazel_dep + git_override` in MODULE.bazel (so its transitive deps,
including @pybind11_bazel, cascade through BCR). WORKSPACE declares it
directly in WORKSPACE and chains `whisper_dependencies()`.

For @opensrc-axi and @opensrc-common_cells we override the upstream
BUILD.bazel with a minimal `build_file_content` rather than fetch the
full BUILD graph — the originals reach @tensix-tt_tech / @bzsim which we
no longer ship, and we only need a thin verilog_library slice anyway.

TODO(open-source): swap the pulp-platform forks (axi, common_cells,
nlohmann-json) for the public github upstreams; re-publish the
Tenstorrent-owned forks (CoreArchChecker, mem_manager).
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def _declare_cvm_and_rules_hdl():
    # One pinned cvm tarball, fetched twice — once for @cvm (full tree),
    # once for @rules_hdl (the rules_hdl_compat shim sub-dir).
    commit = "438b90fdfa6c8449e124e756f0ddc392cf2fe93e"
    url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/{c}/cvm-{c}.tar.bz2".format(c = commit)
    sha256 = "7e1a0f6e137936634e68b3fc5327b58e0c0823295275c2ba97ee9f65aefcc3d4"
    strip_prefix = "cvm-" + commit
    http_archive(name = "cvm", urls = [url], strip_prefix = strip_prefix, sha256 = sha256)
    http_archive(name = "rules_hdl", urls = [url], strip_prefix = strip_prefix + "/bazel/rules_hdl_compat", sha256 = sha256)

_OPENSRC_AXI_BUILD = """
load("@rules_hdl//verilog:providers.bzl", "verilog_library")

verilog_library(
    name = "axi",
    srcs = ["src/axi_pkg.sv"],
    hdrs = [
        "include/axi/assign.svh",
        "include/axi/port.svh",
        "include/axi/typedef.svh",
    ],
    # rules_hdl_compat treats strip_include_prefix as the "namespace prefix"
    # the include directive prepends — here `\\\\`include "axi/typedef.svh"`
    # adds "axi/", so we strip "axi" off the file's parent dir to leave
    # `external/opensrc-axi/include/` as the include search path.
    strip_include_prefix = "axi",
    visibility = ["//visibility:public"],
)
"""

_OPENSRC_COMMON_CELLS_BUILD = """
load("@rules_hdl//verilog:providers.bzl", "verilog_library")

verilog_library(
    name = "clk_mux_glitch_free",
    srcs = ["src/clk_mux_glitch_free.sv"],
    visibility = ["//visibility:public"],
)
"""

def rv_tester_external_deps():
    _declare_cvm_and_rules_hdl()

    http_archive(
        name = "CoreArchChecker",
        sha256 = "cf897b3945d6aafe3a00831bf33adf004f3e1821020aab806ccc27c601ebf311",
        strip_prefix = "CoreArchChecker-d1d38af45ebddf8aa54a510b2b62196f19555109",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/d1d38af45ebddf8aa54a510b2b62196f19555109/CoreArchChecker-d1d38af45ebddf8aa54a510b2b62196f19555109.tar.bz2",
    )

    http_archive(
        name = "mem_manager",
        sha256 = "fc1d138f26405bac05f039a945fb004fc081ea49c16aa7553d9e1b0b49a4d338",
        strip_prefix = "mem-manager-585efffd1a79f43388339c193cdae420f32acad4",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/mem-manager/-/archive/585efffd1a79f43388339c193cdae420f32acad4/mem-manager-585efffd1a79f43388339c193cdae420f32acad4.tar.bz2",
    )

    git_repository(
        name = "opensrc-nlohmann-json",
        commit = "ece38f1883dd1e59c498c63b8f53c3b4bcbc593c",
        remote = "https://aus-gitlab.local.tenstorrent.com/opensrc/opensrc-nlohmann-json.git",
    )

    # pulp-platform/axi fork. BUILD overridden — see _OPENSRC_AXI_BUILD.
    # We pull typedef.svh + axi_pkg.sv only; the upstream BUILD pulls in
    # @opensrc-common_cells/@tensix-tt_tech/@bzsim which we don't ship.
    git_repository(
        name = "opensrc-axi",
        commit = "7387de678e025b809341fa8a2893ef5e931e6d4d",
        shallow_since = "1669784673 -0600",
        remote = "https://aus-gitlab.local.tenstorrent.com/opensrc/opensrc-axi.git",
        build_file_content = _OPENSRC_AXI_BUILD,
    )

    # pulp-platform/common_cells fork. BUILD overridden — see
    # _OPENSRC_COMMON_CELLS_BUILD. We only need clk_mux_glitch_free.sv on
    # the smoke path (referenced from rv_tester_clkgen).
    git_repository(
        name = "opensrc-common_cells",
        commit = "8c522859dd3b996c9b6d5fca4906b9fe6987006c",
        shallow_since = "1658763133 -0500",
        remote = "https://aus-gitlab.local.tenstorrent.com/opensrc/opensrc-common_cells.git",
        build_file_content = _OPENSRC_COMMON_CELLS_BUILD,
    )
