"""Fetches openc910 from GitHub, applies RVFI edits via Python script
(deterministic, whitespace-robust), overlays RVFI sources, and installs BUILD."""

_OPENC910_REMOTE = "https://github.com/XUANTIE-RV/openc910.git"
_OPENC910_COMMIT = "b91c90914c19f114d35c8f6b73408eb241ed847c"

def _git(ctx, args, cwd = "", what = ""):
    res = ctx.execute(["git"] + args, working_directory = cwd, timeout = 1200)
    if res.return_code != 0:
        fail("openc910 fetch: git {} failed ({}):\n{}\n{}".format(what or str(args), res.return_code, res.stdout, res.stderr))
    return res

def _openc910_repo_impl(ctx):
    _git(ctx, ["init", "-q"], what = "init")
    _git(ctx, ["remote", "add", "origin", ctx.attr.remote], what = "remote add")
    _git(ctx, ["fetch", "-q", "--depth", "1", "origin", ctx.attr.commit], what = "fetch")
    _git(ctx, ["-c", "advice.detachedHead=false", "checkout", "-q", "FETCH_HEAD"], what = "checkout")

    # Overlay RVFI sources into stable path.
    for f in ctx.attr.rvfi_srcs:
        ctx.symlink(f, "rvfi/rtl/" + f.name)

    # Apply RVFI edits before header copy so sysmap.v carries remapped PMA.
    res = ctx.execute(["python3", ctx.path(ctx.attr.rvfi_script), ctx.path(".")], timeout = 600)
    if res.return_code != 0:
        fail("openc910 fetch: apply_rvfi.py failed ({}):\n{}\n{}".format(res.return_code, res.stdout, res.stderr))

    # Copy define-only config headers to .v (rules_hdl needs compilable sources);
    # they must stay first in openc910_core srcs so the defines persist.
    for h in ["C910_RTL_FACTORY/gen_rtl/cpu/rtl/cpu_cfig", "C910_RTL_FACTORY/gen_rtl/mmu/rtl/sysmap"]:
        ctx.execute(["cp", h + ".h", h + ".v"], timeout = 60)

    ctx.symlink(ctx.attr.build_file, "BUILD.bazel")

_openc910_repo = repository_rule(
    implementation = _openc910_repo_impl,
    attrs = {
        "remote": attr.string(mandatory = True),
        "commit": attr.string(mandatory = True),
        "build_file": attr.label(mandatory = True, allow_single_file = True),
        "rvfi_script": attr.label(mandatory = True, allow_single_file = True),
        "rvfi_srcs": attr.label_list(allow_files = True),
    },
)

def _ext_impl(_ctx):
    _openc910_repo(
        name = "openc910",
        remote = _OPENC910_REMOTE,
        commit = _OPENC910_COMMIT,
        build_file = "//test/examples/openc910/bazel:openc910.BUILD",
        rvfi_script = "//test/examples/openc910/rtl:apply_rvfi.py",
        rvfi_srcs = ["//test/examples/openc910/rtl/rvfi:ct_rvfi_gen.v"],
    )

openc910_ext = module_extension(implementation = _ext_impl)
