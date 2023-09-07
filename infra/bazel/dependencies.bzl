load("@corearchcoverage//infra/bazel:repositories.bzl", "corearchcoverage_dependencies")
load("@axi-wrapper//:defs.bzl", "axi_wrapper_repositories")
load("@axi_llc-wrapper//:defs.bzl", "axi_llc_wrapper_repositories")
load("@checkin-script//:deps.bzl", "checkin_script_deps")

def rv_tester_dependencies():
    corearchcoverage_dependencies()
    axi_wrapper_repositories()
    axi_llc_wrapper_repositories()
    checkin_script_deps()
