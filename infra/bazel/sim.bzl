# Loading local sim target 

load("@bzsim//:sim.bzl", bzsim_sim_test_suite ="sim_test_suite")
load("@bzsim//:sim.bzl", bzsim_sim_test ="sim_test")
load("@bzsim//:sim.bzl", bzsim_sim_runnable ="sim_runnable")


def sim_test_suite(**kwargs):
    bzsim_sim_test_suite(
        sim = "//infra/bzsim-config:risc_p_cores_sim",
        **kwargs
    )

def sim_test(**kwargs):
    bzsim_sim_test(**kwargs)


def sim_runnable(**kwargs):
    bzsim_sim_runnable(**kwargs)