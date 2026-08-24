# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

"""Golden test for scripts/csr/csr_param_gen.py.

Generates the C++ header and SystemVerilog package from the tiny spec (with
and without the project override) and compares the output byte-for-byte
against checked-in goldens. Regenerate goldens after an intentional
generator change with:

    bazel-7 run //test/csr_param_gen:regen_goldens --config=bzlmod
"""

import os
import unittest

from python.runfiles import Runfiles

import csr_param_gen  # via imports=["."] on //scripts/csr:csr_param_gen_lib

PKG = "test/csr_param_gen"


def rloc(rel_path):
    """Resolve a main-repo runfile under bzlmod (_main) or WORKSPACE (rv_tester) mode."""
    r = Runfiles.Create()
    for root in ("_main", "rv_tester"):
        p = r.Rlocation(root + "/" + rel_path)
        if p and os.path.exists(p):
            return p
    # Test cwd is <runfiles>/<main repo dir>, so a relative path is the fallback.
    if os.path.exists(rel_path):
        return os.path.abspath(rel_path)
    raise FileNotFoundError(rel_path)


def read(path):
    with open(path, "r") as f:
        return f.read()


class CsrParamGenGoldenTest(unittest.TestCase):
    maxDiff = None  # show full diffs on golden mismatch

    def _generate(self, override):
        out_dir = os.environ.get("TEST_TMPDIR", "/tmp")
        hpp = os.path.join(out_dir, "out.hpp")
        sv = os.path.join(out_dir, "out.sv")
        m = csr_param_gen.CsrMap(rloc(PKG + "/data/tiny_csr_spec.yaml"), override)
        m.generate_hpp_file(hpp)
        m.generate_sv_file(sv)
        return read(hpp), read(sv)

    def test_with_override_matches_golden(self):
        hpp, sv = self._generate(rloc(PKG + "/data/tiny_project_override.yaml"))
        self.assertEqual(hpp, read(rloc(PKG + "/golden/tiny_csr_param.hpp")))
        self.assertEqual(sv, read(rloc(PKG + "/golden/tiny_csr_param.sv")))

    def test_no_override_matches_golden(self):
        hpp, sv = self._generate(None)
        self.assertEqual(hpp, read(rloc(PKG + "/golden/tiny_csr_param_no_override.hpp")))
        self.assertEqual(sv, read(rloc(PKG + "/golden/tiny_csr_param_no_override.sv")))


if __name__ == "__main__":
    unittest.main()
