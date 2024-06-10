load("@corearchcoverage//infra/bazel:repositories.bzl", "corearchcoverage_dependencies")
load("@checkin-script//:deps.bzl", "checkin_script_deps")

def rv_tester_dependencies():
    corearchcoverage_dependencies()
    checkin_script_deps()
