"""Rule that turns a project_overrides YAML into a C++ header library.

The generated header (project_overrides.h) is consumed by
cosim/bridge/bridge_params.h, making the project overrides available to the
bridge model as compile-time constants. The YAML is supplied by the caller
(rv_tester_gen), so a user may point it at a different overrides file.
"""

def _project_overrides_gen_impl(ctx):
    header = ctx.outputs.out

    args = ctx.actions.args()
    args.add("--project_overrides", ctx.file.project_overrides)
    args.add("--output", header)

    ctx.actions.run(
        arguments = [args],
        executable = ctx.executable._gen,
        inputs = [ctx.file.project_overrides],
        outputs = [header],
        mnemonic = "ProjectOverridesGen",
        progress_message = "Generating project_overrides header from %s" % ctx.file.project_overrides.short_path,
    )

    return [DefaultInfo(files = depset([header]))]

_project_overrides_gen = rule(
    implementation = _project_overrides_gen_impl,
    attrs = {
        "project_overrides": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "Path to the project_overrides YAML file",
        ),
        "out": attr.output(
            mandatory = True,
            doc = "Generated C++ header",
        ),
        "_gen": attr.label(
            default = "@rv_tester//src/cosim:project_overrides_gen",
            executable = True,
            cfg = "exec",
        ),
    },
    provides = [DefaultInfo],
)

def project_overrides_gen(name, project_overrides, visibility = None, cc_attrs = {}, **kwargs):
    """Generate a cc_library exposing project_overrides.h from a YAML file.

    The header is exposed as "project_overrides.h" (via strip_include_prefix)
    so cosim/bridge/bridge_params.h can include it by a stable name regardless
    of the target name.

    Args:
        name: Base name; produces cc_library `name` and file rule `name`_gen.
        project_overrides: Label of the project_overrides YAML file.
        visibility: Target visibility.
        cc_attrs: Extra attributes forwarded to the cc_library.
        **kwargs: Additional arguments forwarded to the file-generating rule.
    """
    header = name + "/project_overrides.h"

    _project_overrides_gen(
        name = name + "_gen",
        project_overrides = project_overrides,
        out = header,
        visibility = visibility,
        **kwargs
    )

    native.cc_library(
        name = name,
        hdrs = [header],
        strip_include_prefix = name,
        visibility = visibility,
        **cc_attrs
    )
