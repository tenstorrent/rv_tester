load("@checkin-script//:deps2.bzl", "checkin_script_deps2")
load("@opensrc-axi_llc//:defs2.bzl", "opensrc_axi_llc_repositories2")

def rv_tester_dependencies2():
    checkin_script_deps2()
    opensrc_axi_llc_repositories2()
