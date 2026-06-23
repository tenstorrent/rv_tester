"""First chain of transitive deps for the restored WORKSPACE API.

Runs after `rv_tester_repositories()`. Calls @whisper's and cvm's
WORKSPACE-mode dep wiring, then directly declares @cvm_pypi and
@rv_tester_pypi via pip_parse (skipping the longer
`cvm_toolchains1/2` chain so both pip repos come up in a single stage —
keeps the downstream WORKSPACE down to one extra `dependencies2` call).

`install_deps()` for the two pip repos lives in `dependencies2.bzl`
because their `requirements.bzl` loaders don't exist until the
`pip_parse` calls in this stage's body have run.
"""

load("@whisper//:deps.bzl", "whisper_dependencies")
load("@cvm//deps:repositories.bzl", "cvm_dependencies")
load("@rules_python//python:pip.bzl", "pip_parse")

def rv_tester_dependencies():
    whisper_dependencies()
    cvm_dependencies()

    # cvm_toolchains1() is `pip_parse(name="cvm_pypi", ...)` — inline it
    # so we don't need a separate stage just to bridge into @cvm_pypi.
    pip_parse(
        name = "cvm_pypi",
        requirements_lock = "@cvm//deps:requirements_lock.txt",
    )
    pip_parse(
        name = "rv_tester_pypi",
        requirements_lock = "@rv_tester//bazel:requirements.txt",
    )
