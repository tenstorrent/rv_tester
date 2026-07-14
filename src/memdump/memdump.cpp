// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "memdump.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "src/sysmod/htif/htif.h"
#include "sysmod_plusargs.h"
#include "src/sysmod/sysmod_rpc.h"
#include "transactor.h" // pulls in axi_seqids.hpp (SNOOP_GEN_SEQ_ID); needs to be included exactly once per TU

DEFINE_string(memdump, "",
              "Dump memory regions to files at end of test. "
              "Format: file:start_expr:end_expr[,file:start_expr:end_expr...]. "
              "Each expr is a +/- arithmetic over @symbol tokens (resolved from the loaded ELF) "
              "and numeric literals (0x..., decimal, octal). "
              "Example: +memdump=buf.bin:@buf:@buf+@size,head.bin:@base+0x10:@base+0x100");

REGISTRY_register(memdump, TOP.PLATFORM, cvm::registry::all);

namespace {

std::string popen_capture(const std::string& cmd) {
  std::array<char, 256> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe)
    return result;
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::string strip(const std::string& s) {
  const auto first = s.find_first_not_of(" \t\n\r");
  if (first == std::string::npos)
    return "";
  const auto last = s.find_last_not_of(" \t\n\r");
  return s.substr(first, last - first + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string item;
  std::stringstream ss(s);
  while (std::getline(ss, item, delim))
    out.push_back(item);
  return out;
}

} // namespace

memdump::memdump(cvm::topology::loc_t loc, unsigned) {
  loc_ = loc;
  parse_plusarg();
}

void memdump::configure() {
  if (regions_.empty())
    return;

  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  sysmod_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
  channel_ = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_);

  // Reserve a defer slot now so sysmod's terminate handler will hold the
  // sim-end callback until our dump coroutine completes, regardless of the
  // order in which the htif::terminate_t handlers fire.
  cvm::registry::messenger.call<sysmod_block_terminate>(sysmod_loc_);

  cvm::registry::messenger.connect<htif::terminate_t>(
      cvm::topology::get_from_type("PLATFORM", 0),
      [this](htif::terminate_t) {
        auto* task = +[](memdump* self) -> cvm::messenger::task<void> {
          co_await self->dump_all();
          cvm::registry::messenger.call<sysmod_unblock_terminate>(self->sysmod_loc_);
          co_return;
        };
        cvm::registry::messenger.fork(task, this);
      });
}

bool memdump::lookup_symbol(const std::string& sym, uint64_t& out) {
  if (FLAGS_load.empty()) {
    cvm::log(cvm::ERROR, "Error: [memdump] cannot resolve @{}: +load is empty\n", sym);
    return false;
  }
  auto it = symbol_cache_.find(sym);
  if (it != symbol_cache_.end()) {
    out = it->second;
    return true;
  }
  const std::string cmd = "nm " + FLAGS_load + " | grep -w " + sym;
  const std::string result = popen_capture(cmd);
  for (auto& line : split(result, '\n')) {
    if (line.size() < 16)
      continue;
    try {
      uint64_t addr = std::stoull(line.substr(0, 16), nullptr, 16);
      symbol_cache_[sym] = addr;
      cvm::log(cvm::MEDIUM, "[memdump] resolved @{} -> {:#x}\n", sym, addr);
      out = addr;
      return true;
    } catch (...) {
      continue;
    }
  }
  cvm::log(cvm::ERROR, "Error: [memdump] symbol @{} not found in {}\n", sym, FLAGS_load);
  return false;
}

bool memdump::resolve_token(const std::string& tok, uint64_t& out) {
  const std::string t = strip(tok);
  if (t.empty()) {
    cvm::log(cvm::ERROR, "Error: [memdump] empty token in expression\n");
    return false;
  }
  if (t[0] == '@') {
    return lookup_symbol(t.substr(1), out);
  }
  try {
    out = std::stoull(t, nullptr, 0);
    return true;
  } catch (...) {
    cvm::log(cvm::ERROR, "Error: [memdump] cannot parse numeric literal '{}'\n", t);
    return false;
  }
}

bool memdump::evaluate_expr(const std::string& expr, uint64_t& out) {
  std::vector<std::pair<char, std::string>> terms;
  std::string cur;
  char op = '+';
  for (char c : expr) {
    if (c == '+' || c == '-') {
      terms.emplace_back(op, cur);
      op = c;
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  terms.emplace_back(op, cur);

  uint64_t acc = 0;
  for (auto& [o, raw] : terms) {
    uint64_t v = 0;
    if (!resolve_token(raw, v))
      return false;
    if (o == '+')
      acc += v;
    else
      acc -= v;
  }
  out = acc;
  return true;
}

void memdump::parse_plusarg() {
  if (FLAGS_memdump.empty())
    return;
  for (auto& spec : split(FLAGS_memdump, ',')) {
    auto fields = split(spec, ':');
    if (fields.size() != 3) {
      cvm::log(cvm::ERROR,
               "Error: [memdump] bad spec '{}' (expected file:start:end)\n", spec);
      continue;
    }
    const std::string filename = strip(fields[0]);
    if (filename.empty()) {
      cvm::log(cvm::ERROR, "Error: [memdump] empty filename in spec '{}'\n", spec);
      continue;
    }
    uint64_t start = 0, end = 0;
    if (!evaluate_expr(fields[1], start))
      continue;
    if (!evaluate_expr(fields[2], end))
      continue;
    if (start >= end) {
      cvm::log(cvm::ERROR,
               "Error: [memdump] {} : start {:#x} >= end {:#x}, skipping\n",
               filename, start, end);
      continue;
    }
    regions_.push_back({filename, start, end});
    cvm::log(cvm::MEDIUM,
             "[memdump] queued region: {} [{:#x}, {:#x}) ({} bytes)\n",
             filename, start, end, end - start);
  }
}

cvm::messenger::task<void>
memdump::read_line(uint64_t addr, std::vector<uint8_t>& out) {
  axi::a_no_id_t ar{};
  ar.w = false;
  ar.addr = addr & ~uint64_t(0x3F);
  ar.size = axi::sz_t(6); // 64 bytes/beat
  ar.len = axi::len_t(0); // single beat
  ar.burst = axi::burst_t(0);
  ar.prot = axi::prot_t(2);
  ar.seqid = axi::seqid_t(SNOOP_GEN_SEQ_ID);
  ar.allow_decerr_resp = false;

  axi::id_t id;
  while (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar, id)) {
    co_await cvm::registry::messenger.wait<rv_tester_transactions::sysmod::tick<>>(sysmod_loc_);
  }
  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(
      channel_, [&id](const auto& r) { return r.id == id; });
  out.assign(resp.data.begin(), resp.data.end());
  co_return;
}

cvm::messenger::task<void>
memdump::dump_all() {
  std::vector<uint8_t> chunk;
  for (auto& r : regions_) {
    std::ofstream ofs(r.filename, std::ios::binary | std::ios::trunc);
    if (!ofs) {
      cvm::log(cvm::ERROR, "Error: [memdump] cannot open {} for writing\n", r.filename);
      continue;
    }
    const uint64_t cl_start = r.start & ~uint64_t(0x3F);
    const uint64_t cl_end = (r.end + 0x3F) & ~uint64_t(0x3F);
    for (uint64_t addr = cl_start; addr < cl_end; addr += 64) {
      co_await read_line(addr, chunk);
      const uint64_t lo = std::max(addr, r.start);
      const uint64_t hi = std::min(addr + 64, r.end);
      if (chunk.size() < 64) {
        cvm::log(cvm::ERROR,
                 "Error: [memdump] short read at {:#x} ({} bytes), padding with zeros\n",
                 addr, chunk.size());
        chunk.resize(64, 0);
      }
      ofs.write(reinterpret_cast<const char*>(chunk.data() + (lo - addr)), hi - lo);
    }
    cvm::log(cvm::NONE,
             "[memdump] wrote {} bytes to {} from [{:#x}, {:#x})\n",
             r.end - r.start, r.filename, r.start, r.end);
  }
  co_return;
}
