load("@testgen//:defs.bzl", rr_testlist = "testlist")

TESTLISTS = {
    "smoke": {
        "testlist" : "//sw_testbench/testlists:smoke.py",
    },
    "axi_sw_tb_smoke": {
        "testlist" : "//transactors/axi_sw/test/testlists:smoke.py",
    },
    "rv_tester_delay_resp_tb_smoke": {
        "testlist" : "//rv_tester_delay_resp_tb/testlists:smoke.py",
    },
}

def _testlist(name, testlist, **kwargs):

    rr_testlist(
        name = name,
        testlist = testlist,
        workspace = "@rv_tester",
        **kwargs,
    )

def load_testlists():

    for name,values in TESTLISTS.items():
        _testlist(
            name = name,
            testlist = values['testlist'],
            args = values.get('args', [])
        )
