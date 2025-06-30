load("//:rv_tester.bzl", _rv_tester_gen = "rv_tester_gen")
load("//cosim:cosim.bzl", _cosim_gen = "cosim_gen")
load("//csr:csr_collateral_gen.bzl", _csr_collateral_gen = "csr_collateral_gen")

rv_tester_gen = _rv_tester_gen
cosim_gen = _cosim_gen
csr_collateral_gen = _csr_collateral_gen
