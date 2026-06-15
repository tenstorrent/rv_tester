"""Bzlmod side: a module extension that calls the shared
`rv_tester_external_deps()` declaration. Kept as a thin wrapper so the
WORKSPACE side can `load("//bazel:external_deps.bzl", ...)` directly without
going through an extension."""

load(":external_deps.bzl", "rv_tester_external_deps")

def _impl(_ctx):
    rv_tester_external_deps()

rv_tester_external_deps_ext = module_extension(implementation = _impl)
