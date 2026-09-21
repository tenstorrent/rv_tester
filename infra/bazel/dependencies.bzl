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
load("@rules_python//python:repositories.bzl", "python_register_toolchains")

# Hermetic interpreter used ONLY to resolve pip_parse wheels.
#
# The container's system python3 is 3.13 (Debian trixie), but rules_python
# 0.11's pip bootstrap ships a setuptools whose pkg_resources calls
# pkgutil.ImpImporter -- an API removed in Python 3.12 -- so resolving the
# @*_pypi repos against system python dies with
#   AttributeError: module 'pkgutil' has no attribute 'ImpImporter'
# We point pip_parse at a hermetic CPython 3.9 (via python_interpreter_target
# below) so resolution runs under 3.9 and the C-extension deps (pyyaml,
# bitarray) get matching cp39 wheels. rules_python 0.11 has no portable
# host-interpreter alias, so this names the x86_64-linux repo directly (CI is
# x86_64 linux).
_PY_INTERPRETER = "@python3_9_x86_64-unknown-linux-gnu//:python"

def rv_tester_dependencies():
    whisper_dependencies()
    cvm_dependencies()

    python_register_toolchains(
        name = "python3_9",
        python_version = "3.9",
        ignore_root_user_error = True,
        register_toolchains = False,
    )

    # cvm_toolchains1() is `pip_parse(name="cvm_pypi", ...)` — inline it
    # so we don't need a separate stage just to bridge into @cvm_pypi.
    pip_parse(
        name = "cvm_pypi",
        requirements_lock = "@cvm//deps:requirements_lock.txt",
        python_interpreter_target = _PY_INTERPRETER,
    )
    pip_parse(
        name = "rv_tester_pypi",
        requirements_lock = "@rv_tester//bazel:requirements.txt",
        python_interpreter_target = _PY_INTERPRETER,
    )
