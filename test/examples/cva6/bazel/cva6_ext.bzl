"""Fetches CVA6 from GitHub, initializes core/cvfpu submodule subset (git_repository
cannot init a subset), stages deduplicated include paths, and overlays BUILD."""

_CVA6_REMOTE = "https://github.com/openhwgroup/cva6.git"
_CVA6_COMMIT = "31b45637935e944a0e9b4e2cb14e9bb49c9bdd23"

# "<parent>|<path>", ordered so a nested submodule is fetched after its parent
# (empty parent = repo root).
_SUBMODULES = [
    "|core/cvfpu",
    "core/cvfpu|src/fpu_div_sqrt_mvp",
]

def _git(ctx, args, cwd = "", what = ""):
    res = ctx.execute(["git"] + args, working_directory = cwd, timeout = 1200)
    if res.return_code != 0:
        fail("cva6 fetch: git {} failed ({}):\n{}\n{}".format(what or str(args), res.return_code, res.stdout, res.stderr))
    return res

def _run(ctx, args, what):
    res = ctx.execute(args, timeout = 600)
    if res.return_code != 0:
        fail("cva6 fetch: {} failed ({}):\n{}\n{}".format(what, res.return_code, res.stdout, res.stderr))
    return res

def _cva6_repo_impl(ctx):
    _git(ctx, ["init", "-q"], what = "init")
    _git(ctx, ["remote", "add", "origin", ctx.attr.remote], what = "remote add")
    _git(ctx, ["fetch", "-q", "--depth", "1", "origin", ctx.attr.commit], what = "fetch")
    _git(ctx, ["-c", "advice.detachedHead=false", "checkout", "-q", "FETCH_HEAD"], what = "checkout")
    for pair in ctx.attr.submodules:
        parent, _, path = pair.partition("|")
        _git(ctx, ["submodule", "update", "--init", "--depth", "1", path], cwd = parent, what = "submodule " + path)

    # Stage deduplicated include paths: rules_hdl truncates at first namespace match.
    _run(ctx, ["mkdir", "-p", "bazel_include"], "mkdir bazel_include")
    _run(ctx, ["cp", "-rL", "vendor/pulp-platform/common_cells/include/common_cells", "bazel_include/common_cells"], "stage common_cells include")
    _run(ctx, ["cp", "-rL", "vendor/pulp-platform/axi/include/axi", "bazel_include/axi"], "stage axi include")

    ctx.symlink(ctx.attr.build_file, "BUILD.bazel")

_cva6_repo = repository_rule(
    implementation = _cva6_repo_impl,
    attrs = {
        "remote": attr.string(mandatory = True),
        "commit": attr.string(mandatory = True),
        "submodules": attr.string_list(),
        "build_file": attr.label(mandatory = True, allow_single_file = True),
    },
)

def _ext_impl(_ctx):
    _cva6_repo(
        name = "cva6",
        remote = _CVA6_REMOTE,
        commit = _CVA6_COMMIT,
        submodules = _SUBMODULES,
        build_file = "//test/examples/cva6/bazel:cva6.BUILD",
    )

cva6_ext = module_extension(implementation = _ext_impl)
