# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

"""Rewrites the goldens for csr_param_gen_test.py in the source tree.

Usage: bazel-7 run //test/csr_param_gen:regen_goldens --config=bzlmod
"""

import os
import sys

from python.runfiles import Runfiles

import csr_param_gen

PKG = "test/csr_param_gen"


def rloc(rel_path):
    r = Runfiles.Create()
    for root in ("_main", "rv_tester"):
        p = r.Rlocation(root + "/" + rel_path)
        if p and os.path.exists(p):
            return p
    raise FileNotFoundError(rel_path)


def main():
    ws = os.environ.get("BUILD_WORKSPACE_DIRECTORY")  # set only by `bazel run`
    if not ws:
        sys.exit("run via: bazel-7 run //test/csr_param_gen:regen_goldens --config=bzlmod")
    golden = os.path.join(ws, PKG, "golden")
    os.makedirs(golden, exist_ok=True)

    spec = rloc(PKG + "/data/tiny_csr_spec.yaml")
    override = rloc(PKG + "/data/tiny_project_override.yaml")
    defaults = rloc(PKG + "/data/tiny_csral_defaults.yaml")

    m = csr_param_gen.CsrMap(spec, override)
    m.generate_hpp_file(os.path.join(golden, "tiny_csr_param.hpp"))
    m.generate_sv_file(os.path.join(golden, "tiny_csr_param.sv"))
    config = csr_param_gen.CsralConfig(defaults, m.override_data)
    model = csr_param_gen.CsralModel(m, config)
    model.generate_tables_hpp(os.path.join(golden, "tiny_csral_tables.hpp"))

    m = csr_param_gen.CsrMap(spec, None)
    m.generate_hpp_file(os.path.join(golden, "tiny_csr_param_no_override.hpp"))
    m.generate_sv_file(os.path.join(golden, "tiny_csr_param_no_override.sv"))


if __name__ == "__main__":
    main()
