load("@checkin-script//:deps3.bzl", "checkin_script_deps3")
load("@apb-wrapper//:defs.bzl", "apb_wrapper_repositories")

def rv_tester_dependencies3():
    checkin_script_deps3()
    apb_wrapper_repositories()
