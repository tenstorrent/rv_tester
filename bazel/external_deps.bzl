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

For @opensrc-axi we override the upstream BUILD.bazel with a minimal
`build_file_content` rather than fetch the full BUILD graph — the
original reaches @tensix-tt_tech / @bzsim which we no longer ship, and
we only need a thin verilog_library slice anyway.

Every declaration is `maybe()`-wrapped so a downstream consumer that
pre-declares the same repo (with their own pin) wins; ours becomes a
no-op. This mirrors the historical `bazel/repositories.bzl` pattern
(commit `90936c63`).

TODO(open-source): swap the pulp-platform forks (axi, nlohmann-json) for
the public github upstreams; re-publish the Tenstorrent-owned forks
(CoreArchChecker, mem_manager).
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def _declare_cvm_and_rules_hdl():
    # One pinned cvm tarball, fetched twice — once for @cvm (full tree),
    # once for @rules_hdl (the rules_hdl_compat shim sub-dir).
    cvm_hash = "438b90fdfa6c8449e124e756f0ddc392cf2fe93e"
    sha256 = "7e1a0f6e137936634e68b3fc5327b58e0c0823295275c2ba97ee9f65aefcc3d4"
    cvm_url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/cvm/-/archive/{commit}/cvm-{commit}.tar.bz2".format(commit = cvm_hash)
    maybe(
        http_archive,
        name = "cvm",
        sha256 = sha256,
        strip_prefix = "cvm-{commit}".format(commit = cvm_hash),
        url = cvm_url,
    )
    maybe(
        http_archive,
        name = "rules_hdl",
        sha256 = sha256,
        strip_prefix = "cvm-{commit}/bazel/rules_hdl_compat".format(commit = cvm_hash),
        url = cvm_url,
    )

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

def rv_tester_external_deps():
    _declare_cvm_and_rules_hdl()

    CoreArchChecker_hash = "d1d38af45ebddf8aa54a510b2b62196f19555109"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        sha256 = "cf897b3945d6aafe3a00831bf33adf004f3e1821020aab806ccc27c601ebf311",
        strip_prefix = "CoreArchChecker-{commit}".format(commit = CoreArchChecker_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/{commit}/CoreArchChecker-{commit}.tar.bz2".format(commit = CoreArchChecker_hash),
    )

    mem_manager_hash = "585efffd1a79f43388339c193cdae420f32acad4"
    maybe(
        http_archive,
        name = "mem_manager",
        sha256 = "fc1d138f26405bac05f039a945fb004fc081ea49c16aa7553d9e1b0b49a4d338",
        strip_prefix = "mem-manager-{commit}".format(commit = mem_manager_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/mem-manager/-/archive/{commit}/mem-manager-{commit}.tar.bz2".format(commit = mem_manager_hash),
        patches = ["@rv_tester//bazel:mem_manager_use_bcr_lz4.patch"],
        patch_args = ["-p1"],
    )

    opensrc_nlohmann_json_hash = "ece38f1883dd1e59c498c63b8f53c3b4bcbc593c"
    maybe(
        git_repository,
        name = "opensrc-nlohmann-json",
        commit = opensrc_nlohmann_json_hash,
        remote = "https://aus-gitlab.local.tenstorrent.com/opensrc/opensrc-nlohmann-json.git",
    )

    # pulp-platform/axi fork. BUILD overridden — see _OPENSRC_AXI_BUILD.
    # We pull typedef.svh + axi_pkg.sv only; the upstream BUILD pulls in
    # @opensrc-common_cells/@tensix-tt_tech/@bzsim which we don't ship.
    opensrc_axi_hash = "7387de678e025b809341fa8a2893ef5e931e6d4d"
    maybe(
        git_repository,
        name = "opensrc-axi",
        commit = opensrc_axi_hash,
        shallow_since = "1669784673 -0600",
        remote = "https://aus-gitlab.local.tenstorrent.com/opensrc/opensrc-axi.git",
        build_file_content = _OPENSRC_AXI_BUILD,
    )
