#! /usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

import math

def clog2(x):
    return math.ceil(math.log2(x))

if __name__ == "__main__":
    queues = {
        "R_Q": 1024,
        "B_Q": 1024,
        "AR_Q": 1024,
        "AW_Q": 1024,
        "W_Q" : 1024,
    }

    cfg = {}
    for q,size in queues.items():
        m = 1 << clog2(size + 1)
        cfg.update({
            q + "_MAX": size,
            q + "_PTR_MAX": m,
            q + "_PTR_MAX_LOG2": clog2(m),
        })

    print("axi_sw: &rv_tester_axi_sw")
    for k,v in cfg.items():
        if k.startswith("R_Q") or k.startswith("B_Q"):
            print(f"  {k}: {v}")

    print("axi_sw_mst: &rv_tester_axi_sw_mst")
    for k,v in cfg.items():
        if not k.startswith("R_Q") and not k.startswith("B_Q"):
            print(f"  {k}: {v}")
    # Default AxLEN cap shared by every master so the grouped-AXI topology struct
    # carries the field uniformly (template.sv sizes it from the first group); a
    # bus that must cap bursts (e.g. an AXI-Lite port) overrides this to 0. 255 is
    # the full AXI range, which the mmr_txn_router reroute path treats as no cap.
    print("  MAX_AXLEN: 255")
