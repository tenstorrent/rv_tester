"""Stub for @pybind11_bazel//:build_defs.bzl needed by whisper's BUILD load line.

whisper's `pybind_extension` target builds the Python bindings to the ISS,
which the smoke testbench doesn't use. We create a no-op rule so the target
declaration succeeds at load time; nothing actually depends on the produced
artifact in the open-source slice. TODO(open-source): drop once whisper has
a public bzlmod-friendly fork.
"""

def _noop_impl(ctx):
    return [DefaultInfo()]

_noop = rule(
    implementation = _noop_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "hdrs": attr.label_list(allow_files = True),
        "deps": attr.label_list(),
        "copts": attr.string_list(),
        "linkopts": attr.string_list(),
        "defines": attr.string_list(),
    },
)

def pybind_extension(name, **kwargs):
    _noop(
        name = name,
        srcs = kwargs.get("srcs", []),
        hdrs = kwargs.get("hdrs", []),
        deps = kwargs.get("deps", []),
        copts = kwargs.get("copts", []),
        linkopts = kwargs.get("linkopts", []),
        defines = kwargs.get("defines", []),
        visibility = kwargs.get("visibility", []),
    )

def pybind_library(name, **kwargs):
    _noop(
        name = name,
        srcs = kwargs.get("srcs", []),
        hdrs = kwargs.get("hdrs", []),
        deps = kwargs.get("deps", []),
        copts = kwargs.get("copts", []),
        linkopts = kwargs.get("linkopts", []),
        defines = kwargs.get("defines", []),
        visibility = kwargs.get("visibility", []),
    )
