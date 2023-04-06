load("@testgen//:defs.bzl", rr_testlist = "testlist")

TESTLISTS = {
    "mem_manager_example_smoke": {
        "testlist" : "//dv/tb/mem_manager_example/testlists:smoke.list",
    },
    "info_pass_smoke": {
        "testlist" : "//dv/tb/info_pass/testlists:smoke.list",
    },
    "fe_smoke": {
        "testlist" : "//dv/fe/testlists:smoke.list",
    },
    "mc_smoke": {
        "testlist" : "//dv/mc/testlists:smoke.py",
    },
    "mc_dummy": {
        "testlist" : "//dv/mc/testlists:dummy.list",
    },
    "ls_smoke": {
        "testlist" : "//dv/ls/testlists:smoke.py",
    },
    "vec_smoke": {
        "testlist" : "//dv/vec/testlists:smoke.py",
    },
    "core_smoke": {
        "testlist" : "//dv/core/testlists:smoke.py",
    },
}
def _testlist(name, testlist, **kwargs):

    rr_testlist(
        name = name,
        testlist = testlist,
        workspace = "@risc-p-cores",
        **kwargs,
    )

def load_testlists():

    for name,values in TESTLISTS.items():
        _testlist(
            name = name,
            testlist = values['testlist'],
            args = values.get('args', [])
        )
