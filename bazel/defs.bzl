load("//src:rv_tester.bzl", _rv_tester_gen = "rv_tester_gen")
load("//src/cosim:cosim.bzl", _cosim_gen = "cosim_gen")
load("//scripts/csr:csr_param_gen.bzl", _csr_param_gen = "csr_param_gen")

rv_tester_gen = _rv_tester_gen
cosim_gen = _cosim_gen
csr_param_gen = _csr_param_gen
