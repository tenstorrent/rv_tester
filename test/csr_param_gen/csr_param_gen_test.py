# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

"""Golden + validation tests for scripts/csr/csr_param_gen.py.

Golden part: generates the C++ header, SystemVerilog package, and CSRAL
tables from the tiny spec and compares byte-for-byte against checked-in
goldens. Regenerate after an intentional generator change with:

    bazel-7 run //test/csr_param_gen:regen_goldens --config=bzlmod

Validation part: builds small malformed specs/overrides inline and asserts
generation fails with the right message (a bad spec or a typo'd policy must
fail the BUILD, never a simulation).
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


def tmpdir():
    return os.environ.get("TEST_TMPDIR", "/tmp")


class CsrParamGenGoldenTest(unittest.TestCase):
    maxDiff = None  # show full diffs on golden mismatch

    def _generate(self, override):
        hpp = os.path.join(tmpdir(), "out.hpp")
        sv = os.path.join(tmpdir(), "out.sv")
        m = csr_param_gen.CsrMap(rloc(PKG + "/data/tiny_csr_spec.yaml"), override)
        m.generate_hpp_file(hpp)
        m.generate_sv_file(sv)
        return m, read(hpp), read(sv)

    def test_with_override_matches_golden(self):
        m, hpp, sv = self._generate(rloc(PKG + "/data/tiny_project_override.yaml"))
        self.assertEqual(hpp, read(rloc(PKG + "/golden/tiny_csr_param.hpp")))
        self.assertEqual(sv, read(rloc(PKG + "/golden/tiny_csr_param.sv")))
        # CSRAL tables from the same run (tiny defaults + the override's csral section).
        config = csr_param_gen.CsralConfig(rloc(PKG + "/data/tiny_csral_defaults.yaml"), m.override_data)
        model = csr_param_gen.CsralModel(m, config)
        tables = os.path.join(tmpdir(), "out_tables.hpp")
        model.generate_tables_hpp(tables)
        self.assertEqual(read(tables), read(rloc(PKG + "/golden/tiny_csral_tables.hpp")))

    def test_no_override_matches_golden(self):
        _, hpp, sv = self._generate(None)
        self.assertEqual(hpp, read(rloc(PKG + "/golden/tiny_csr_param_no_override.hpp")))
        self.assertEqual(sv, read(rloc(PKG + "/golden/tiny_csr_param_no_override.sv")))


class CsralValidationTest(unittest.TestCase):
    """Each case builds a minimal bad input and asserts the failure message."""

    BASE_SPEC = """
good:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "3:0", RESET_VALUE: "0x1"}
"""
    BASE_DEFAULTS = """
compare_mask_source: whisper
reset_check: error
policies:
  default: {check: true, on_mismatch: error}
"""

    def _write(self, name, content):
        path = os.path.join(tmpdir(), name)
        with open(path, "w") as f:
            f.write(content)
        return path

    def _run(self, spec_yaml=None, override_yaml=None, defaults_yaml=None):
        spec = self._write("spec.yaml", spec_yaml or self.BASE_SPEC)
        override = self._write("override.yaml", override_yaml) if override_yaml else None
        m = csr_param_gen.CsrMap(spec, override)
        config = csr_param_gen.CsralConfig(self._write("defaults.yaml", defaults_yaml or self.BASE_DEFAULTS), m.override_data)
        return csr_param_gen.CsralModel(m, config)

    def _expect(self, regex, **kwargs):
        with self.assertRaisesRegex(csr_param_gen.CsralValidationError, regex):
            self._run(**kwargs)

    def test_missing_fields_range_fails_parse(self):
        spec = self._write("spec.yaml", "bad:\n  common_data: {ADDRESS: \"0x100\"}\n  f: {RESET_VALUE: \"0x0\"}\n")
        with self.assertRaisesRegex(ValueError, "FIELDS_RANGE"):
            csr_param_gen.CsrMap(spec, None)

    def test_msb_lsb_swapped_fails_parse(self):
        spec = self._write("spec.yaml", "bad:\n  common_data: {ADDRESS: \"0x100\"}\n  f: {FIELDS_RANGE: \"0:3\"}\n")
        with self.assertRaisesRegex(ValueError, "msb < lsb"):
            csr_param_gen.CsrMap(spec, None)

    def test_field_exceeds_csr_size(self):
        self._expect("exceeds CSR_SIZE", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 32}
  f: {FIELDS_RANGE: "35:32", RESET_VALUE: "0x0"}
""")

    def test_overlapping_fields(self):
        self._expect("overlaps", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64}
  a: {FIELDS_RANGE: "7:0", RESET_VALUE: "0x0"}
  b: {FIELDS_RANGE: "8:4", RESET_VALUE: "0x0"}
""")

    def test_reset_does_not_fit_field(self):
        self._expect("does not fit", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "3:0", RESET_VALUE: "0x1F"}
""")

    def test_duplicate_address(self):
        self._expect("already used", spec_yaml=self.BASE_SPEC + """
clash:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0"}
""")

    def test_alias_target_missing(self):
        self._expect("ALIAS_OF 'nothere'", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64, ALIAS_OF: nothere}
  f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0"}
""")

    def test_field_masked_by_without_register(self):
        self._expect("no common_data MASKED_BY", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0", MASKED_BY: EN}
""")

    def test_gate_register_missing(self):
        self._expect("not a CSR in this spec", spec_yaml="""
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64, MASKED_BY: ghostreg}
  f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0", MASKED_BY: EN}
""")

    def test_gate_field_missing(self):
        self._expect("not a field of masking register", spec_yaml="""
gate:
  common_data: {ADDRESS: "0x101", CSR_SIZE: 64}
  other: {FIELDS_RANGE: "0", RESET_VALUE: "0x0"}
bad:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64, MASKED_BY: gate}
  f: {FIELDS_RANGE: "1", RESET_VALUE: "0x0", MASKED_BY: EN}
""")

    def test_masked_by_cycle(self):
        self._expect("cycle", spec_yaml="""
a:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64, MASKED_BY: b}
  fa: {FIELDS_RANGE: "0", RESET_VALUE: "0x0", MASKED_BY: fb}
b:
  common_data: {ADDRESS: "0x101", CSR_SIZE: 64, MASKED_BY: a}
  fb: {FIELDS_RANGE: "0", RESET_VALUE: "0x0", MASKED_BY: fa}
""")

    def test_project_policy_unknown_name_is_error(self):
        self._expect("matches no CSR", override_yaml="""
csral:
  policies:
    not_a_csr: {on_mismatch: skip}
""")

    def test_defaults_policy_unknown_name_only_warns(self):
        model = self._run(defaults_yaml=self.BASE_DEFAULTS + """
  not_a_csr: {on_mismatch: skip}
""")
        self.assertTrue(any("matches no CSR" in w for w in model.warnings))

    # Spec keys are not uniformly cased (the cores spell DSCRATCH0/1 in upper
    # case), so every name lookup MUST ignore case or a policy silently misses.
    UPPER_SPEC = """
DSCRATCH0:
  common_data: {ADDRESS: "0x7B2", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "63:0", RESET_VALUE: "0x0"}
dpc:
  common_data: {ADDRESS: "0x7B1", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "63:0", RESET_VALUE: "0x0"}
"""

    def test_defaults_policy_binds_regardless_of_case(self):
        model = self._run(spec_yaml=self.UPPER_SPEC, defaults_yaml=self.BASE_DEFAULTS + """
  dscratch0: {on_mismatch: skip}
""")
        self.assertEqual(model.policy_by_csr["DSCRATCH0"]["on_mismatch"], "skip")
        self.assertFalse(any("matches no CSR" in w for w in model.warnings), model.warnings)

    def test_project_glob_binds_regardless_of_case(self):
        model = self._run(spec_yaml=self.UPPER_SPEC, override_yaml="""
csral:
  policies:
    "dscr*": {on_mismatch: skip}
    DPC: {volatile: true}
""")
        self.assertEqual(model.policy_by_csr["DSCRATCH0"]["on_mismatch"], "skip")
        self.assertTrue(model.policy_by_csr["dpc"]["volatile"])

    def test_policy_patterns_differing_only_by_case_rejected(self):
        with self.assertRaisesRegex(csr_param_gen.CsralValidationError, "differ only by case"):
            self._run(spec_yaml=self.UPPER_SPEC, override_yaml="""
csral:
  policies:
    dpc: {on_mismatch: skip}
    DPC: {volatile: true}
""")

    def test_spec_names_differing_only_by_case_rejected(self):
        self._expect("differ only by case", spec_yaml=self.UPPER_SPEC + """
Dpc:
  common_data: {ADDRESS: "0x7B4", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "63:0", RESET_VALUE: "0x0"}
""")

    def test_field_alias_names_bind_regardless_of_case(self):
        model = self._run(spec_yaml=self.UPPER_SPEC, defaults_yaml=self.BASE_DEFAULTS + """
field_aliases:
  - {csr: dscratch0, bits: "3:0", alias_of: DPC, alias_bits: "3:0"}
""")
        self.assertEqual(model.field_alias_rows, [("DSCRATCH0", 3, 0, "dpc", 3, 0)])
        self.assertFalse(any("field_aliases" in w for w in model.warnings), model.warnings)

    def test_exists_if_condition_binds_regardless_of_case(self):
        model = self._run(spec_yaml="""
GATE:
  common_data: {ADDRESS: "0x101", CSR_SIZE: 64}
  en: {FIELDS_RANGE: "0", RESET_VALUE: "0x0"}
dep:
  common_data: {ADDRESS: "0x100", CSR_SIZE: 64, MASKED_BY: GATE}
  f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0", MASKED_BY: en}
""", override_yaml="""
csral:
  policies:
    dep: {exists_if: gate.EN}
""")
        self.assertEqual(model.policy_by_csr["dep"]["exists_if_index"], 0)

    def test_real_defaults_exempt_per_hart_reset(self):
        # mhartid resets to the hart index; the spec holds one value, so the
        # whisper-vs-spec reset check MUST skip it or every hart > 0 fails at init.
        model = self._run(spec_yaml=self.BASE_SPEC + """
mhartid:
  common_data: {ADDRESS: "0xF14", CSR_SIZE: 64}
  f: {FIELDS_RANGE: "63:0", RESET_VALUE: "0x0"}
""", defaults_yaml=read(rloc("scripts/csr/csral_defaults.yaml")))
        self.assertFalse(model.policy_by_csr["mhartid"]["check_reset"])
        self.assertTrue(model.policy_by_csr["good"]["check_reset"])

    def test_cac_check_override_unknown_name(self):
        self._expect("cac_check_overrides", override_yaml="cac_check_overrides:\n  not_a_csr: true\n")

    def test_save_restore_in_yaml_rejected(self):
        with self.assertRaisesRegex(csr_param_gen.CsralValidationError, "csral_save_restore"):
            self._run(override_yaml="csral:\n  save_restore:\n    misa.H: true\n")

    def test_unknown_policy_key_rejected(self):
        self._expect("unknown keys", override_yaml="csral:\n  policies:\n    good: {resync: true}\n")

    def test_ambiguous_globs_rejected(self):
        self._expect("different policies", defaults_yaml=self.BASE_DEFAULTS + """
  "go*": {on_mismatch: skip}
  "g*d": {on_mismatch: resynch_rd}
""")

    def test_conflicting_additional_spellings(self):
        spec = self._write("spec.yaml", self.BASE_SPEC)
        override = self._write("override.yaml", """
addtional_csrs:
  x: {common_data: {ADDRESS: "0x200"}, f: {FIELDS_RANGE: "0"}}
additional_csrs:
  y: {common_data: {ADDRESS: "0x201"}, f: {FIELDS_RANGE: "0"}}
""")
        with self.assertRaisesRegex(ValueError, "different content"):
            csr_param_gen.CsrMap(spec, override)

    def test_correct_additional_spelling_accepted(self):
        spec = self._write("spec.yaml", self.BASE_SPEC)
        override = self._write("override.yaml", """
additional_csrs:
  extra:
    common_data: {ADDRESS: "0x200", CSR_SIZE: 64}
    f: {FIELDS_RANGE: "0", RESET_VALUE: "0x0"}
""")
        m = csr_param_gen.CsrMap(spec, override)
        self.assertIn("extra", m.csr_property_dict)


if __name__ == "__main__":
    unittest.main()
