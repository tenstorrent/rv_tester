load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def aplic_monitor_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):
        
    aplic_monitor_dpi = name + "_dpi"
    aplic_monitor_sv = name + "_sv"

    native.cc_library(
		name = aplic_monitor_dpi,
		srcs = [
		"@rv_tester//aplic_monitor:aplic_monitor.cpp"
		],
		hdrs = [
		"@rv_tester//aplic_monitor:aplic_monitor.hpp",
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
		name = aplic_monitor_sv,
		srcs = ["@rv_tester//aplic_monitor:aplic_monitor.sv"],
		deps = [
		"@cvm//:plusargs_sv",
		"@cvm//:topology_sv",
		packet + "_sv",
		topology + "_sv",
		harness,
		],
		visibility = visibility,
	)