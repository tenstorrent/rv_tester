"""Second chain — installs the @cvm_pypi and @rv_tester_pypi packages.

Runs after `rv_tester_dependencies()`. `install_deps()` materializes the
per-package subrepos (@cvm_pypi_pyyaml, @rv_tester_pypi_pyyaml, …) that
@cvm and @rv_tester targets reference.
"""

load("@cvm_pypi//:requirements.bzl", cvm_install_deps = "install_deps")
load("@rv_tester_pypi//:requirements.bzl", rv_tester_install_deps = "install_deps")

def rv_tester_dependencies2():
    cvm_install_deps()
    rv_tester_install_deps()
