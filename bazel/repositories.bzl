"""Restored legacy WORKSPACE-mode API.

Downstream consumers (Bazel-6 WORKSPACE-mode) use:

    load("@rv_tester//bazel:repositories.bzl", "rv_tester_repositories")
    rv_tester_repositories()
    load("@rv_tester//infra/bazel:dependencies.bzl", "rv_tester_dependencies")
    rv_tester_dependencies()
    load("@rv_tester//infra/bazel:dependencies2.bzl", "rv_tester_dependencies2")
    rv_tester_dependencies2()

`rv_tester_repositories()` declares every external repo rv_tester's load
chain needs at the WORKSPACE-mode entry point — @cvm, @rules_hdl,
@CoreArchChecker, @mem_manager, @opensrc-*, @rules_verilog, @whisper, and
@rules_python (eager so the dependencies.bzl top-level `load()` of
`@rules_python//python:pip.bzl` resolves).

The two `rv_tester_dependencies*` macros chain the secondary loaders:
`rv_tester_dependencies()` calls `cvm_dependencies()` + `whisper_dependencies()`
+ `pip_parse(rv_tester_pypi)`; `rv_tester_dependencies2()` calls
`install_deps()` from @rv_tester_pypi. Two stages are unavoidable because
Starlark requires `load()` at file scope and @rv_tester_pypi only exists
after the `pip_parse()` call in stage 1's body.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//bazel:external_deps.bzl", "rv_tester_external_deps")

def rv_tester_repositories(_unused_bzlmod = False):
    """Declare rv_tester's external repos.

    The `_unused_bzlmod` keyword exists for source-compat with consumers
    that historically called `rv_tester_repositories(bzlmod=False)`. Under
    bzlmod the macro is not used at all (MODULE.bazel handles everything),
    so the flag has no effect.

    All declarations below are `maybe()`-wrapped so a downstream consumer
    that pre-declares the same repo (with their own pin) wins; ours
    becomes a no-op. Historical first-declaration-wins pattern.
    """
    rv_tester_external_deps()

    # rules_verilog supplies the upstream VerilogInfo symbol rules_hdl_compat
    # re-exports. Same pin cvm uses.
    maybe(
        http_archive,
        name = "rules_verilog",
        url = "https://github.com/hw-bzl/bazel_rules_verilog/releases/download/v1.1.0/bazel_rules_verilog-1.1.0.tar.gz",
        sha256 = "043196310d1ba692ec217c3778663da0d232a3746ba6291d3a12d6461de24021",
        strip_prefix = "bazel_rules_verilog-1.1.0",
    )

    # @whisper has its own MODULE.bazel + deps.bzl; under WORKSPACE we
    # declare it directly and chain whisper_dependencies() in
    # `rv_tester_dependencies()` below.
    maybe(
        git_repository,
        name = "whisper",
        commit = "b50d320a0182a1bd6a869ec35acad1b19697c8f7",
        shallow_since = "2026-07-15",
        remote = "https://github.com/tenstorrent/whisper.git",
    )

    # Eager-declare @rules_python so `infra/bazel/dependencies.bzl` can
    # top-level `load("@rules_python//python:pip.bzl", "pip_parse")`.
    # Without this we'd have to add an extra dependencies stage just to
    # bridge into pip_parse. Same pin cvm uses (cvm_dependencies()'s
    # `maybe(http_archive, name = "rules_python", ...)` is a no-op once
    # this declaration wins).
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "c03246c11efd49266e8e41e12931090b613e12a59e6f55ba2efd29a7cb8b4258",
        strip_prefix = "rules_python-0.11.0",
        url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.11.0.tar.gz",
    )
