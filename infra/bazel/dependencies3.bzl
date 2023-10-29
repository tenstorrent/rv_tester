load("@checkin-script//:deps3.bzl", "checkin_script_deps3")
load("@opensrc-axi_llc//:defs3.bzl", "opensrc_axi_llc_repositories3")

def rv_tester_dependencies3():
    checkin_script_deps3()
    opensrc_axi_llc_repositories3()
