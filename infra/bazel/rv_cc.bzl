
def _rv_cc_bin_impl(ctx):

    args = ctx.actions.args()

    args.add("compile")
    args.add_all(ctx.files.srcs)

    output = ctx.actions.declare_file(
        ctx.attr.name + '.o'
    )

    args.add("--output", output.path)

    ctx.actions.run(
        inputs     = ctx.files.srcs,
        outputs    = [output],
        executable = ctx.executable._sim,
        arguments  = [args],
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(files = depset([output])),
    ]

rv_cc_bin = rule(
    implementation = _rv_cc_bin_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = [".s", ".S", ".c", ".json"],
        ),
        "_sim": attr.label(
            default = "@bzsim//:sim",
            executable = True,
            cfg = "target",
        ),
    },
)
