load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def reset_driver_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):
        
    reset_driver_dpi = name + "_dpi"
    reset_driver_sv = name + "_sv"

    native.cc_library(
		name = reset_driver_dpi,
		srcs = [
		"@rv_tester//reset_driver:reset_driver.cpp"
		],
		hdrs = [
		"@rv_tester//reset_driver:reset_driver.hpp",
		],
		deps = [
		packet + "_cc",
		"@cvm//:plusargs",
		"@cvm//:logger",
		"@cvm//:registry",
		"@cvm//:bitmanip",
		"@aplic//:Aplic",
		],
		alwayslink = True,
		visibility = visibility,
	)

    verilog_library(
		name = reset_driver_sv,
		srcs = ["@rv_tester//reset_driver:reset_driver.sv"],
		deps = [
		"@cvm//:plusargs_sv",
		"@cvm//:topology_sv",
		packet + "_sv",
		topology + "_sv",
		harness,
		],
		visibility = visibility,
	)
