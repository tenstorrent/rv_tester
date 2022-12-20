load("//:repositories.bzl", "rv_tester_repositories")
rv_tester_repositories()

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("@rules_python//python:pip.bzl", "pip_parse")
pip_parse(
    name = "pypi",
    # (Optional) You can set quiet to False if you want to see pip output.
    #quiet = False,
    requirements_lock = "@cvm//py:requirements_lock.txt",
)

load("@pypi//:requirements.bzl", "install_deps")
# Initialize repositories for all packages in requirements_lock.txt.
install_deps()
