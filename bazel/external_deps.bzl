"""Shared declaration of rv_tester's external aus-gitlab dependencies.

Holds repos that DON'T have their own MODULE.bazel (no bzlmod-native
dep wiring). Called from two places:
- `MODULE.bazel` via the module extension in `:external_deps_ext.bzl` so
  bzlmod resolves @CoreArchChecker / @mem_manager / @opensrc-nlohmann-json
  (and `use_repo(...)` exposes each name).
- `WORKSPACE` via `load("//bazel:external_deps.bzl",
  "rv_tester_external_deps") + rv_tester_external_deps()` so Bazel-6
  WORKSPACE-mode resolves the same set.

@whisper is NOT here — it has its own MODULE.bazel so bzlmod takes it via
`bazel_dep + git_override` in MODULE.bazel (so its transitive deps,
including @pybind11_bazel, cascade through BCR). WORKSPACE declares it
directly in WORKSPACE and chains `whisper_dependencies()`.

TODO(open-source): mirror each of these onto a public source
(nlohmann/json from BCR for the JSON parser; re-publish the
Tenstorrent-owned forks).
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def rv_tester_external_deps():
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
