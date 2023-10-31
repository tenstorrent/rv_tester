load("@corearchcoverage//infra/bazel:repositories.bzl", "corearchcoverage_dependencies")
load("@opensrc-axi_llc//:defs.bzl", "opensrc_axi_llc_repositories")
load("@checkin-script//:deps.bzl", "checkin_script_deps")

def rv_tester_dependencies():
    corearchcoverage_dependencies()
    opensrc_axi_llc_repositories()
    checkin_script_deps()
