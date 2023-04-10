load("@testgen//:defs.bzl", rr_testlist = "testlist")

TESTLISTS = {
    "smoke": {
        "testlist" : "//dv/testlists:smoke.py",
    },
}

def _testlist(name, testlist, **kwargs):

    rr_testlist(
        name = name,
        testlist = testlist,
        workspace = "@chips",
        **kwargs,
    )

def load_testlists():

    for name,values in TESTLISTS.items():
        _testlist(
            name = name,
            testlist = values['testlist'],
            args = values.get('args', [])
        )
