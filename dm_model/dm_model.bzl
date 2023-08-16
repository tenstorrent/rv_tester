load("@rules_hdl//verilog:providers.bzl", "verilog_library")

def dm_model_gen(name, packet, topology, harness, visibility = None, cc_attrs = {}, **kwargs):
        
    dm_model_dpi = name + "_dpi"
    dm_model_sv = name + "_sv"

    native.cc_library(
		name = dm_model_dpi,
		srcs = [
		"@rv_tester//dm_model:dm_model.cpp"
		],
		hdrs = [
		"@rv_tester//dm_model:dm_model.hpp",
		"@rv_tester//dm_model:decode.h",
		"@rv_tester//dm_model:encoding.h",
		"@rv_tester//dm_model:opcodes.h",
		"@rv_tester//dm_model:processor.h",
		"@rv_tester//dm_model:debug_defines.h",
		"@rv_tester//dm_model:debug_rom.h",
		"@rv_tester//dm_model:debug_rom_defines.h",
		],
		deps = [
		packet + "_cc",
		"@cvm//:plusargs",
		"@cvm//:logger",
		"@cvm//:registry",
		],
		alwayslink = True,
		visibility = visibility,
	)

    verilog_library(
		name = dm_model_sv,
		srcs = ["@rv_tester//dm_model:dm_model.sv"],
		deps = [
		"@cvm//:plusargs_sv",
		"@cvm//:topology_sv",
		packet + "_sv",
		topology + "_sv",
		harness,
		],
		visibility = visibility,
	)