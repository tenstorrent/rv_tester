load("@rules_hdl//verilog:providers.bzl", "verilog_library")
load("//infra/bazel:verilog.bzl", "verilog_unit")

def aclint_checker_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):
        
    aclint_checker_dpi = name + "_dpi"
    aclint_checker_sv = name + "_sv"

    native.cc_library(
		name = aclint_checker_dpi,
		srcs = [
		"@rv_tester//aclint_checker:aclint_checker.cpp"
		],
		hdrs = [
		"@rv_tester//aclint_checker:aclint_checker.hpp",
		],
		deps = [
		packet + "_cc",
		"@cvm//:plusargs",
		"@cvm//:logger",
		"@cvm//:registry",
		"@cvm//:bitmanip",
		],
		alwayslink = True,
		visibility = visibility,
	)

    verilog_library(
		name = aclint_checker_sv,
		srcs = ["@rv_tester//aclint_checker:aclint_checker.sv"],
		deps = [
		"@cvm//:plusargs_sv",
		"@cvm//:topology_sv",
		packet + "_sv",
		topology + "_sv",
		harness,
		],
		visibility = visibility,
	)

    verilog_library(
        name = "assertion_macros",
        srcs = ["assertion_macros.vh", "sva_macros.vh"],
        visibility = ["//visibility:public"],
    )