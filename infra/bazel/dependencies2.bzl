load("@checkin-script//:deps2.bzl", "checkin_script_deps2")
load("@common_cells-wrapper//:defs.bzl", "common_cells_wrapper_repositories")
load("@common_verification-wrapper//:defs.bzl", "common_verification_wrapper_repositories")
load("@tech_cells_generic-wrapper//:defs.bzl", "tech_cells_generic_wrapper_repositories")
load("@register_interface-wrapper//:defs.bzl", "register_interface_wrapper_repositories")

def rv_tester_dependencies2():
    checkin_script_deps2()
    common_cells_wrapper_repositories()
    common_verification_wrapper_repositories()
    tech_cells_generic_wrapper_repositories()
    register_interface_wrapper_repositories()
