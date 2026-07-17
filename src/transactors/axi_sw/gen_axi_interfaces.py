#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

"""
Generate AXI interface definitions from topology configuration.
This script creates SystemVerilog macros and typedefs for all AXI interfaces.
"""

import argparse
import yaml
import json
import re
import sys
import os

DEFAULT_AXI_SW_TOPO = 'TOP.PLATFORM.AXI_SW'
DEFAULT_AXI_SW_MST_TOPO = 'TOP.PLATFORM.AXI_SW_MST'
DEFAULT_CLK_IDX_MAP = {1: 'CORE_CLK_IDX', 2: 'AXI_CLK_IDX', 3: 'SOC_CLK_IDX'}


class AXIInterfaceGenerator:

    def __init__(self,
                 topology_json=None,
                 transaction_file=None,
                 axi_sw_topo=DEFAULT_AXI_SW_TOPO,
                 axi_sw_mst_topo=DEFAULT_AXI_SW_MST_TOPO,
                 clk_idx_map=None):
        self.topology_json = topology_json
        self.transaction_file = transaction_file
        self.axi_sw_topo = axi_sw_topo
        self.axi_sw_mst_topo = axi_sw_mst_topo
        self.clk_idx_map = clk_idx_map if clk_idx_map is not None else dict(DEFAULT_CLK_IDX_MAP)
        self.axi_sw_interfaces = dict()
        self.axi_sw_mst_interfaces = dict()

    def parse_topology(self):

        with open(self.topology_json, 'r') as f:
            topology = json.load(f)

        for axi_kind, target in ((self.axi_sw_topo,     self.axi_sw_interfaces),
                                 (self.axi_sw_mst_topo, self.axi_sw_mst_interfaces)):
            for i, intf in enumerate(topology.get(axi_kind, [])):
                name = intf['NAME'].lower()

                intf.setdefault('SHARD', 1)
                intf['INDEX'] = i
                target[name] = intf

    def generate_sv(self):
        lines = []
        lines.append("")
        lines.append('`include "axi/typedef.svh"')
        lines.append("")

        lines.append("`define RV_TESTER_AXI_ENUMS \\")
        enum_lines = []

        if self.axi_sw_interfaces:
            enum_lines.append("    typedef enum int {")
            for name, intf in self.axi_sw_interfaces.items():
                enum_lines.append(f"        {name.upper()}_IDX = {intf['INDEX']},")
            enum_lines[-1] = enum_lines[-1].rstrip(',')
            enum_lines.append("    } axi_sw_idx_e;")

        if self.axi_sw_mst_interfaces:
            enum_lines.append("    typedef enum int {")
            for name, intf in self.axi_sw_mst_interfaces.items():
                enum_lines.append(f"        {name.upper()}_IDX = {intf['INDEX']},")
            enum_lines[-1] = enum_lines[-1].rstrip(',')
            enum_lines.append("    } axi_sw_mst_idx_e;")

        for i, line in enumerate(enum_lines):
            if i < len(enum_lines) - 1:
                lines.append(line + " \\")
            else:
                lines.append(line)

        lines.append("")
        lines.append("`define RV_TESTER_AXI_TYPEDEFS \\")

        typedef_lines = []
        for name, intf in self.axi_sw_interfaces.items():
            typedef_lines.extend(self.generate_interface_typedefs(name, intf))
        for name, intf in self.axi_sw_mst_interfaces.items():
            typedef_lines.extend(self.generate_interface_typedefs(name, intf))

        for i, line in enumerate(typedef_lines):
            if i < len(typedef_lines) - 1:
                lines.append(line + " \\")
            else:
                lines.append(line)

        lines.append("")

        lines.extend(self.generate_port_macro("RV_TESTER_AXI"))
        lines.extend(self.generate_instantiation_macros())
        lines.append("")

        return '\n'.join(lines)

    def generate_interface_typedefs(self, name, intf):
        lines = []

        lines.append(f"    typedef logic [{intf['ADDR_WIDTH']}-1:0] {name}_addr_t;")
        lines.append(f"    typedef logic [{intf['DATA_WIDTH']}-1:0] {name}_data_t;")
        lines.append(f"    typedef logic [{intf['STRB_WIDTH']}-1:0] {name}_strb_t;")
        lines.append(f"    typedef logic [{intf['ID_WIDTH']}-1:0] {name}_id_t;")
        default_user_width = 8 if 'mst' in name else 1
        user_width = intf.get('USER_WIDTH', default_user_width)
        lines.append(f"    typedef logic [{user_width}-1:0] {name}_user_t;")
        lines.append("")

        lines.append(f"    `AXI_TYPEDEF_AW_CHAN_T({name}_aw_chan_t, {name}_addr_t, {name}_id_t, {name}_user_t)")
        lines.append(f"    `AXI_TYPEDEF_W_CHAN_T({name}_w_chan_t, {name}_data_t, {name}_strb_t, {name}_user_t)")
        lines.append(f"    `AXI_TYPEDEF_B_CHAN_T({name}_b_chan_t, {name}_id_t, {name}_user_t)")
        lines.append(f"    `AXI_TYPEDEF_AR_CHAN_T({name}_ar_chan_t, {name}_addr_t, {name}_id_t, {name}_user_t)")
        lines.append(f"    `AXI_TYPEDEF_R_CHAN_T({name}_r_chan_t, {name}_data_t, {name}_id_t, {name}_user_t)")
        lines.append(f"    `AXI_TYPEDEF_REQ_T({name}_req_t, {name}_aw_chan_t, {name}_w_chan_t, {name}_ar_chan_t)")
        lines.append(f"    `AXI_TYPEDEF_RESP_T({name}_rsp_t, {name}_b_chan_t, {name}_r_chan_t)")
        lines.append("")

        return lines

    def generate_port_macro(self, macro_name):
        lines = []

        port_lines = []
        var_lines = []

        for name, intf in self.axi_sw_interfaces.items():
            hi = intf['SHARD'] - 1
            var_lines.append(f"    pkg::{name}_req_t {name}_req [{hi}:0]; \\")
            var_lines.append(f"    pkg::{name}_rsp_t {name}_rsp [{hi}:0]; \\")
            port_lines.append(f"    output pkg::{name}_req_t {name}_req [{hi}:0], \\")
            port_lines.append(f"    input  pkg::{name}_rsp_t {name}_rsp [{hi}:0], \\")

        for name, intf in self.axi_sw_mst_interfaces.items():
            hi = intf['SHARD'] - 1
            var_lines.append(f"    pkg::{name}_req_t {name}_req [{hi}:0]; \\")
            var_lines.append(f"    pkg::{name}_rsp_t {name}_rsp [{hi}:0]; \\")
            port_lines.append(f"    input  pkg::{name}_req_t {name}_req [{hi}:0], \\")
            port_lines.append(f"    output pkg::{name}_rsp_t {name}_rsp [{hi}:0], \\")

        if port_lines:
            port_lines[-1] = port_lines[-1].rstrip(', \\')
        if var_lines:
            var_lines[-1] = var_lines[-1].rstrip(';\\').rstrip('; \\') + ';'

        lines.append(f"`define {macro_name}_PORTS(input, output, pkg) \\")
        lines.extend(port_lines)
        lines.append("")

        lines.append(f"`define {macro_name}_VARS(pkg) \\")
        lines.extend(var_lines)
        lines.append("")

        return lines

    def generate_instantiation_macros(self, macro_suffix=""):
        lines = []
        lines.append(f"`define AXI_INSTANTIATION{macro_suffix} \\")
        for name, intf in self.axi_sw_interfaces.items():
            domain = intf.get('DOMAIN', 2)
            clk_idx = self.clk_idx_map.get(domain, 'AXI_CLK_IDX')
            sh = intf['SHARD']
            NAME = name.upper()

            lines.append(f"    for (genvar k = 0; k < {sh}; k++) begin : g_{name} \\")
            lines.append(f"    axi_sw #( \\")
            lines.append(f"        .ADDR_WIDTH({intf['ADDR_WIDTH']}), \\")
            lines.append(f"        .DATA_WIDTH({intf['DATA_WIDTH']}), \\")
            lines.append(f"        .ID_WIDTH({intf['ID_WIDTH']}), \\")
            lines.append(f"        .STRB_WIDTH({intf['STRB_WIDTH']}), \\")
            lines.append(f"        .R_Q_MAX({intf.get('R_Q_MAX', 1024)}), \\")
            lines.append(f"        .B_Q_MAX({intf.get('B_Q_MAX', 1024)}), \\")
            lines.append(f"        .LOCATION(cvm_topology_gen::get_location(topology.{self.axi_sw_topo}[{NAME}_IDX].ID, k)), \\")
            lines.append(f"        .tag(\"{name}\"), \\")
            lines.append(f"        `RV_TESTER_TRANSACTIONS_{NAME}_SOURCE_PARAMS(0) \\")
            lines.append(f"    ) {name} ( \\")
            lines.append(f"        .clk(dut_clk[{clk_idx}]), \\")
            lines.append(f"        .sys_reset(sys_reset[{clk_idx}]), \\")
            lines.append(f"        .reset_n(~dut_reset[{clk_idx}]), \\")
            lines.append(f"        /* Connect all AW channel signals */ \\")
            lines.append(f"        .axi_mst_aw_valid({name}_req[k].aw_valid), \\")
            lines.append(f"        .axi_mst_aw_id({name}_req[k].aw.id), \\")
            lines.append(f"        .axi_mst_aw_addr({name}_req[k].aw.addr), \\")
            lines.append(f"        .axi_mst_aw_len({name}_req[k].aw.len), \\")
            lines.append(f"        .axi_mst_aw_size({name}_req[k].aw.size), \\")
            lines.append(f"        .axi_mst_aw_burst({name}_req[k].aw.burst), \\")
            lines.append(f"        .axi_mst_aw_lock({name}_req[k].aw.lock), \\")
            lines.append(f"        .axi_mst_aw_cache({name}_req[k].aw.cache), \\")
            lines.append(f"        .axi_mst_aw_prot({name}_req[k].aw.prot), \\")
            lines.append(f"        .axi_mst_aw_qos({name}_req[k].aw.qos), \\")
            lines.append(f"        .axi_mst_aw_region({name}_req[k].aw.region), \\")
            lines.append(f"        .axi_mst_aw_atop({name}_req[k].aw.atop), \\")
            lines.append(f"        .axi_mst_aw_user({name}_req[k].aw.user), \\")
            lines.append(f"        /* Connect all AR channel signals */ \\")
            lines.append(f"        .axi_mst_ar_valid({name}_req[k].ar_valid), \\")
            lines.append(f"        .axi_mst_ar_id({name}_req[k].ar.id), \\")
            lines.append(f"        .axi_mst_ar_addr({name}_req[k].ar.addr), \\")
            lines.append(f"        .axi_mst_ar_len({name}_req[k].ar.len), \\")
            lines.append(f"        .axi_mst_ar_size({name}_req[k].ar.size), \\")
            lines.append(f"        .axi_mst_ar_lock({name}_req[k].ar.lock), \\")
            lines.append(f"        .axi_mst_ar_burst({name}_req[k].ar.burst), \\")
            lines.append(f"        .axi_mst_ar_cache({name}_req[k].ar.cache), \\")
            lines.append(f"        .axi_mst_ar_prot({name}_req[k].ar.prot), \\")
            lines.append(f"        .axi_mst_ar_qos({name}_req[k].ar.qos), \\")
            lines.append(f"        .axi_mst_ar_region({name}_req[k].ar.region), \\")
            lines.append(f"        .axi_mst_ar_user({name}_req[k].ar.user), \\")
            lines.append(f"        /* Connect all W channel signals */ \\")
            lines.append(f"        .axi_mst_w_valid({name}_req[k].w_valid), \\")
            lines.append(f"        .axi_mst_w_data({name}_req[k].w.data), \\")
            lines.append(f"        .axi_mst_w_strb({name}_req[k].w.strb), \\")
            lines.append(f"        .axi_mst_w_last({name}_req[k].w.last), \\")
            lines.append(f"        /* Connect ready signals */ \\")
            lines.append(f"        .axi_mst_b_ready({name}_req[k].b_ready), \\")
            lines.append(f"        .axi_mst_r_ready({name}_req[k].r_ready), \\")
            lines.append(f"        /* Connect response channels */ \\")
            lines.append(f"        .axi_slv_b_valid({name}_rsp[k].b_valid), \\")
            lines.append(f"        .axi_slv_b_id({name}_rsp[k].b.id), \\")
            lines.append(f"        .axi_slv_b_resp({name}_rsp[k].b.resp), \\")
            lines.append(f"        .axi_slv_r_valid({name}_rsp[k].r_valid), \\")
            lines.append(f"        .axi_slv_r_id({name}_rsp[k].r.id), \\")
            lines.append(f"        .axi_slv_r_data({name}_rsp[k].r.data), \\")
            lines.append(f"        .axi_slv_r_resp({name}_rsp[k].r.resp), \\")
            lines.append(f"        .axi_slv_r_last({name}_rsp[k].r.last), \\")
            lines.append(f"        .axi_slv_aw_ready({name}_rsp[k].aw_ready), \\")
            lines.append(f"        .axi_slv_ar_ready({name}_rsp[k].ar_ready), \\")
            lines.append(f"        .axi_slv_w_ready({name}_rsp[k].w_ready), \\")
            lines.append(f"        `RV_TESTER_TRANSACTIONS_{NAME}_SOURCE_PORTS({domain},k,0) \\")
            lines.append(f"    ); \\")
            lines.append(f"    end \\")
            lines.append(f"    \\")

        for name, intf in self.axi_sw_mst_interfaces.items():
            domain = intf.get('DOMAIN', 2)
            clk_idx = self.clk_idx_map.get(domain, 'AXI_CLK_IDX')
            sh = intf['SHARD']
            NAME = name.upper()

            lines.append(f"    for (genvar k = 0; k < {sh}; k++) begin : g_{name} \\")
            lines.append(f"    axi_sw_mst #( \\")
            lines.append(f"        .ADDR_WIDTH({intf['ADDR_WIDTH']}), \\")
            lines.append(f"        .DATA_WIDTH({intf['DATA_WIDTH']}), \\")
            lines.append(f"        .ID_WIDTH({intf['ID_WIDTH']}), \\")
            lines.append(f"        .STRB_WIDTH({intf['STRB_WIDTH']}), \\")
            lines.append(f"        .USER_WIDTH({intf.get('USER_WIDTH', 8)}), \\")
            lines.append(f"        .AR_Q_MAX({intf.get('AR_Q_MAX', 1024)}), \\")
            lines.append(f"        .AW_Q_MAX({intf.get('AW_Q_MAX', 1024)}), \\")
            lines.append(f"        .W_Q_MAX({intf.get('W_Q_MAX', 1024)}), \\")
            lines.append(f"        .LOCATION(cvm_topology_gen::get_location(topology.{self.axi_sw_mst_topo}[{NAME}_IDX].ID, k)), \\")
            lines.append(f"        .tag(\"{name}\"), \\")
            lines.append(f"        `RV_TESTER_TRANSACTIONS_{NAME}_SOURCE_PARAMS(0) \\")
            lines.append(f"    ) {name} ( \\")
            lines.append(f"        .clk(dut_clk[{clk_idx}]), \\")
            lines.append(f"        .sys_reset(sys_reset[{clk_idx}]), \\")
            lines.append(f"        .reset_n(~dut_reset[{clk_idx}]), \\")
            lines.append(f"        /* Connect all AW channel signals */ \\")
            lines.append(f"        .axi_mst_aw_valid({name}_req[k].aw_valid), \\")
            lines.append(f"        .axi_mst_aw_id({name}_req[k].aw.id), \\")
            lines.append(f"        .axi_mst_aw_addr({name}_req[k].aw.addr), \\")
            lines.append(f"        .axi_mst_aw_len({name}_req[k].aw.len), \\")
            lines.append(f"        .axi_mst_aw_size({name}_req[k].aw.size), \\")
            lines.append(f"        .axi_mst_aw_burst({name}_req[k].aw.burst), \\")
            lines.append(f"        .axi_mst_aw_lock({name}_req[k].aw.lock), \\")
            lines.append(f"        .axi_mst_aw_cache({name}_req[k].aw.cache), \\")
            lines.append(f"        .axi_mst_aw_prot({name}_req[k].aw.prot), \\")
            lines.append(f"        .axi_mst_aw_qos({name}_req[k].aw.qos), \\")
            lines.append(f"        .axi_mst_aw_region({name}_req[k].aw.region), \\")
            lines.append(f"        .axi_mst_aw_atop({name}_req[k].aw.atop), \\")
            lines.append(f"        .axi_mst_aw_user({name}_req[k].aw.user), \\")
            lines.append(f"        /* Connect all AR channel signals */ \\")
            lines.append(f"        .axi_mst_ar_valid({name}_req[k].ar_valid), \\")
            lines.append(f"        .axi_mst_ar_id({name}_req[k].ar.id), \\")
            lines.append(f"        .axi_mst_ar_addr({name}_req[k].ar.addr), \\")
            lines.append(f"        .axi_mst_ar_len({name}_req[k].ar.len), \\")
            lines.append(f"        .axi_mst_ar_size({name}_req[k].ar.size), \\")
            lines.append(f"        .axi_mst_ar_lock({name}_req[k].ar.lock), \\")
            lines.append(f"        .axi_mst_ar_burst({name}_req[k].ar.burst), \\")
            lines.append(f"        .axi_mst_ar_cache({name}_req[k].ar.cache), \\")
            lines.append(f"        .axi_mst_ar_prot({name}_req[k].ar.prot), \\")
            lines.append(f"        .axi_mst_ar_qos({name}_req[k].ar.qos), \\")
            lines.append(f"        .axi_mst_ar_region({name}_req[k].ar.region), \\")
            lines.append(f"        .axi_mst_ar_user({name}_req[k].ar.user), \\")
            lines.append(f"        /* Connect all W channel signals */ \\")
            lines.append(f"        .axi_mst_w_valid({name}_req[k].w_valid), \\")
            lines.append(f"        .axi_mst_w_data({name}_req[k].w.data), \\")
            lines.append(f"        .axi_mst_w_strb({name}_req[k].w.strb), \\")
            lines.append(f"        .axi_mst_w_last({name}_req[k].w.last), \\")
            lines.append(f"        /* Connect ready signals */ \\")
            lines.append(f"        .axi_mst_b_ready({name}_req[k].b_ready), \\")
            lines.append(f"        .axi_mst_r_ready({name}_req[k].r_ready), \\")
            lines.append(f"        /* Connect response channels */ \\")
            lines.append(f"        .axi_slv_b_valid({name}_rsp[k].b_valid), \\")
            lines.append(f"        .axi_slv_b_id({name}_rsp[k].b.id), \\")
            lines.append(f"        .axi_slv_b_resp({name}_rsp[k].b.resp), \\")
            lines.append(f"        .axi_slv_r_valid({name}_rsp[k].r_valid), \\")
            lines.append(f"        .axi_slv_r_id({name}_rsp[k].r.id), \\")
            lines.append(f"        .axi_slv_r_data({name}_rsp[k].r.data), \\")
            lines.append(f"        .axi_slv_r_resp({name}_rsp[k].r.resp), \\")
            lines.append(f"        .axi_slv_r_last({name}_rsp[k].r.last), \\")
            lines.append(f"        .axi_slv_aw_ready({name}_rsp[k].aw_ready), \\")
            lines.append(f"        .axi_slv_ar_ready({name}_rsp[k].ar_ready), \\")
            lines.append(f"        .axi_slv_w_ready({name}_rsp[k].w_ready),\\")
            lines.append(f"        `RV_TESTER_TRANSACTIONS_{NAME}_SOURCE_PORTS({domain},k,0) \\")
            lines.append(f"    ); \\")
            lines.append(f"    end \\")
            lines.append(f"    \\")

        lines.append("")
        return lines

    def generate_defines_header(self):
        lines = ['#pragma once', '']
        for name, intf in self.axi_sw_interfaces.items():
            lines.append(f"#define {name.upper()}_IDX {intf['INDEX']}")
            lines.append(f"#define {name.upper()}_PATH \"{self.axi_sw_topo}[{intf['INDEX']}]\"")
        for name, intf in self.axi_sw_mst_interfaces.items():
            lines.append(f"#define {name.upper()}_IDX {intf['INDEX']}")
            lines.append(f"#define {name.upper()}_PATH \"{self.axi_sw_mst_topo}[{intf['INDEX']}]\"")
        lines.append('')
        return '\n'.join(lines)

    def generate_cpp(self):
        sw_lines = [
            '#include "axi_sw.h"',
            '#include "rv_tester_transactions.hpp"',
            '#include "axi_defines.h"',
            '',
        ]
        mst_lines = [
            '#include "axi_sw_mst.h"',
            '#include "rv_tester_transactions.hpp"',
            '#include "axi_defines.h"',
            '',
        ]

        for name, intf in self.axi_sw_interfaces.items():
            ns = f"rv_tester_transactions::{name}"
            sw_lines.append(f"REGISTRY_register((axi_sw<{ns}::w<>,")
            sw_lines.append(f"                         {ns}::aw<>,")
            sw_lines.append(f"                         {ns}::ar<>,")
            sw_lines.append(f"                         {ns}::r_q_ptr<>,")
            sw_lines.append(f"                         {ns}::b_q_ptr<>>), {self.axi_sw_topo}[{name.upper()}_IDX], cvm::registry::all);")
            sw_lines.append("")

        for name, intf in self.axi_sw_mst_interfaces.items():
            ns = f"rv_tester_transactions::{name}"
            mst_lines.append(f"REGISTRY_register((axi_sw_mst<{ns}::b<>,")
            mst_lines.append(f"                              {ns}::r<>,")
            mst_lines.append(f"                              {ns}::ar_q_ptr<>,")
            mst_lines.append(f"                              {ns}::aw_q_ptr<>,")
            mst_lines.append(f"                              {ns}::w_q_ptr<>>), {self.axi_sw_mst_topo}[{name.upper()}_IDX], cvm::registry::all);")
            mst_lines.append("")

        return '\n'.join(sw_lines), '\n'.join(mst_lines)

def main():
    parser = argparse.ArgumentParser(description='Generate AXI interface definitions from topology and transactions')
    parser.add_argument('--topology', required=True, help='Path to topology JSON file')
    parser.add_argument('--sv-output', required=True, help='Output SystemVerilog file')
    parser.add_argument('--sw-cpp-output', required=False, help='Output cpp file for axi_sw REGISTRY_register calls')
    parser.add_argument('--mst-cpp-output', required=False, help='Output cpp file for axi_sw_mst REGISTRY_register calls')
    parser.add_argument('--defines-output', required=False, help='Output C++ header with AXI loc IDX defines')
    parser.add_argument('--axi-sw-topo', default=DEFAULT_AXI_SW_TOPO, help=f'Topology path for AXI_SW group (default: {DEFAULT_AXI_SW_TOPO})')
    parser.add_argument('--axi-sw-mst-topo', default=DEFAULT_AXI_SW_MST_TOPO, help=f'Topology path for AXI_SW_MST group (default: {DEFAULT_AXI_SW_MST_TOPO})')
    parser.add_argument('--clk-idx-map', default=None, help='JSON object mapping DOMAIN ints to SV clock-index identifiers ' f'(default: {json.dumps({str(k): v for k, v in DEFAULT_CLK_IDX_MAP.items()})})')

    args = parser.parse_args()

    clk_idx_map = None
    if args.clk_idx_map:
        clk_idx_map = {int(k): v for k, v in json.loads(args.clk_idx_map).items()}

    generator = AXIInterfaceGenerator(
        topology_json=args.topology,
        axi_sw_topo=args.axi_sw_topo,
        axi_sw_mst_topo=args.axi_sw_mst_topo,
        clk_idx_map=clk_idx_map,
    )

    try:
        generator.parse_topology()
        sv_content = generator.generate_sv()

        with open(args.sv_output, 'w') as f:
            f.write(sv_content)

        if args.sw_cpp_output and args.mst_cpp_output:
            sw_content, mst_content = generator.generate_cpp()
            with open(args.sw_cpp_output, 'w') as f:
                f.write(sw_content)
            with open(args.mst_cpp_output, 'w') as f:
                f.write(mst_content)

        if args.defines_output:
            with open(args.defines_output, 'w') as f:
                f.write(generator.generate_defines_header())

    except Exception as e:
        print(f"Error generating AXI interfaces: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
