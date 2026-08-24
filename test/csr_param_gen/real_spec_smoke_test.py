# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

"""Generates from the real rv_tester CSR spec + project overrides + CSRAL
defaults and asserts the derived model's key invariants. This is where a spec
edit, a policy typo, or a generator regression fails first."""

import os
import unittest

from python.runfiles import Runfiles

import csr_param_gen


def rloc(rel_path):
    r = Runfiles.Create()
    for root in ("_main", "rv_tester"):
        p = r.Rlocation(root + "/" + rel_path)
        if p and os.path.exists(p):
            return p
    if os.path.exists(rel_path):
        return os.path.abspath(rel_path)
    raise FileNotFoundError(rel_path)


class RealSpecSmokeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = csr_param_gen.CsrMap(rloc("scripts/csr/csr_spec.yaml"), rloc("scripts/project_overrides.yaml"))
        config = csr_param_gen.CsralConfig(rloc("scripts/csr/csral_defaults.yaml"), cls.m.override_data)
        cls.model = csr_param_gen.CsralModel(cls.m, config)

    def test_generates_all_outputs(self):
        out = os.environ.get("TEST_TMPDIR", "/tmp")
        self.m.generate_hpp_file(os.path.join(out, "real.hpp"))
        self.m.generate_sv_file(os.path.join(out, "real.sv"))
        self.model.generate_tables_hpp(os.path.join(out, "real_tables.hpp"))

    def test_misa_h_condition_matches_spec(self):
        # The misa.H condition must exist and carry the SPEC-derived masks.
        # Note: these deliberately differ from two buggy hand-coded literals
        # in bridge.h (mstatus 0x0000000300000000 masks UXL, medeleg 0xF1000
        # disagrees with its own comment); the spec values below are the
        # architecturally correct ones (MPV|GVA, medeleg_3|medeleg_masked_0).
        cond = {c["name"]: c for c in self.model.conditions}
        self.assertIn("misa.H", cond)
        h = cond["misa.H"]
        self.assertEqual(h["gate_mask"], 0x80)
        targets = dict(h["targets"])
        self.assertEqual(targets["mstatus"], 0x000000C000000000)
        self.assertEqual(targets["medeleg"], 0x0000000000F00400)
        for name in ("mideleg", "mip", "mie"):
            self.assertEqual(targets[name], 0x1444)

    def test_delegation_views_are_view_only(self):
        for c in self.model.conditions:
            if c["gate_csr"] in ("mideleg", "hideleg"):
                self.assertTrue(c["view_only"], c["name"])

    def test_legacy_skip_list_maps_to_policies(self):
        pol = self.model.policy_by_csr
        self.assertEqual(pol["mstatus"]["on_mismatch"], "skip")
        self.assertEqual(pol["mip"]["on_mismatch"], "resynch_rd")
        self.assertTrue(pol["mip"]["volatile"])
        self.assertTrue(pol["mip"]["interrupt"])
        self.assertTrue(pol["vstopei"]["may_not_exist"])
        self.assertFalse(pol["c_fecfg2"]["check"])  # c_* default
        self.assertEqual(pol["mscratch"]["on_mismatch"], "error")  # untouched default

    def test_field_aliases_resolved(self):
        # fcsr <-> fflags/frm shifted views from the defaults.
        pairs = {(c, a) for (c, _m, _l, a, _am, _al) in self.model.field_alias_rows}
        self.assertIn(("fcsr", "fflags"), pairs)
        self.assertIn(("fcsr", "frm"), pairs)


if __name__ == "__main__":
    unittest.main()
