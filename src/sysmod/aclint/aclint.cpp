// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "src/sysmod/aclint/aclint.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "svdpi.h"

// ACLINT MMR offsets and CTIME broadcast target. Defined here (default 0) so
// standalone rv_tester TBs link without an external definition; the cluster
// integration sets the real values via core_dv.flagfile.
DEFINE_uint64(aclint_mtime_offset, 0, "ACLINT mtime MMR offset from device base");
DEFINE_uint64(aclint_timesync_offset, 0, "ACLINT timesync MMR offset from device base");
DEFINE_uint64(aclint_mtimecmp0_offset, 0, "ACLINT mtimecmp0 MMR offset from device base (stride 8 per hart)");
DEFINE_uint64(aclint_ctime_addr, 0, "Core CTIME MMR absolute address for ACLINT time broadcast");

// -----------------------------------------------------------------------------
// SV side of the aclint model (src/sysmod/sysmod.sv). mtime, the MTIP compare
// and the reference pulse live in SV; here we only read/write/broadcast.
// -----------------------------------------------------------------------------
extern "C" {
void sysmod_aclint_set_mtimecmp(unsigned hartid, unsigned long val);
void sysmod_aclint_set_mtime(unsigned long val);
unsigned long sysmod_aclint_get_mtime();
void sysmod_aclint_pulse_timesync();
}

namespace {
svScope g_aclint_sv_scope = nullptr;

// RAII helper: enter the sysmod DPI scope for an SV export call, then restore.
struct scope_guard {
  svScope prev_ = nullptr;
  bool active_ = false;
  explicit scope_guard(svScope s) {
    if (s) {
      prev_ = svSetScope(s);
      active_ = true;
    }
  }
  ~scope_guard() {
    if (active_)
      svSetScope(prev_);
  }
};
} // namespace

// Called once from sysmod.sv (initial block) to capture the sysmod DPI scope.
extern "C" void sysmod_aclint_register_scope() { g_aclint_sv_scope = svGetScope(); }

aclint::aclint(const std::string& tag, uint64_t addr, unsigned hartCount,
               cvm::topology::loc_t loc,
               cvm::topology::loc_t axiMstLoc, uint64_t ctimeAddr)
    : device(tag, addr, 0xc000 /* size */, loc, &aclint::write, &aclint::read, this), hartCount_(hartCount), soft_(hartCount),
      timeCompare_(hartCount, -1),
      axiMstLoc_(axiMstLoc), ctimeAddr_(ctimeAddr) {

  std::ifstream ifs;
  if (load_snapshot(ifs)) {
    std::string line;
    while (std::getline(ifs, line)) {
      // expect aclint format
      std::string type, val;

      std::istringstream iss(line);
      iss >> type;

      if (type == "cmp") {
        iss >> val;
        uint64_t num = strtoull(val.c_str(), nullptr, 0);
        iss >> val;
        // TODO: error check number < hartCount
        timeCompare_.at(num) = strtoull(val.c_str(), nullptr, 0);
      } else {
        cvm::log(cvm::NONE, "Error: unrecognized line " + type + " for " + tag + "\n");
      }
    }

    ifs.close();
  }
}

aclint::~aclint() {
  std::stringstream ss;
  // mtime is free-running in SV; only the compare values are worth saving.
  for (unsigned i = 0; i < timeCompare_.size(); i++) {
    ss << "cmp " << std::dec << i << " " << timeCompare_.at(i) << '\n';
  }

  save_snapshot(ss);
}

void aclint::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

  // Only 4-byte and 8-byte accesses are modeled; ignore all others.
  if (length != 4 && length != 8)
    return;

  uint64_t offset = addr - device::addr();

  // 4-byte accesses target the low/high word of an 8-byte aligned register.
  uint64_t aligned = offset & ~uint64_t(0x7);
  bool hi = (offset & 0x4) != 0;

  uint64_t reg = 0;
  bool matched = false;
  if (aligned == FLAGS_aclint_mtime_offset) {
    scope_guard g(g_aclint_sv_scope);
    reg = sysmod_aclint_get_mtime(); // live mtime from SV.
    matched = true;
  } else if (aligned == FLAGS_aclint_timesync_offset) {
    reg = 0; // write-only sync strobe; reads back 0.
    matched = true;
  } else if (aligned >= FLAGS_aclint_mtimecmp0_offset &&
             aligned < FLAGS_aclint_mtimecmp0_offset + uint64_t(hartCount_) * 8) {
    unsigned hartIx = (aligned - FLAGS_aclint_mtimecmp0_offset) / 8;
    reg = timeCompare_.at(hartIx);
    matched = true;
  }

  if (!matched)
    return;

  if (length == 4)
    reg = hi ? (reg >> 32) : (reg & 0xffffffffULL);
  serializeInt(reg, length, data);
}

void aclint::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;

  // Only 4-byte and 8-byte accesses are modeled; discard all others.
  if (length != 4 && length != 8)
    return;

  uint64_t offset = addr - device::addr();

  // 4-byte accesses target the low/high word of an 8-byte aligned register.
  uint64_t aligned = offset & ~uint64_t(0x7);
  bool hi = (offset & 0x4) != 0;

  // Collect up to 8 write-data bytes (length is 4 or 8).
  uint64_t wdata = 0;
  for (size_t i = 0; i < length && i < 8; ++i)
    wdata |= uint64_t(data[i]) << (i * 8);

  // Merge wdata into a 64-bit register value honoring the access width.
  auto merge = [&](uint64_t old) -> uint64_t {
    if (length == 8)
      return wdata;
    if (hi)
      return (old & 0x00000000ffffffffULL) | ((wdata & 0xffffffffULL) << 32);
    return (old & 0xffffffff00000000ULL) | (wdata & 0xffffffffULL);
  };

  scope_guard g(g_aclint_sv_scope);
  if (aligned == FLAGS_aclint_mtime_offset) {
    // SW mtime write: update the SV counter, then broadcast to core CTIME.
    sysmod_aclint_set_mtime(merge(sysmod_aclint_get_mtime()));
    broadcastTime(); // RTL MtimeUpdate.
  } else if (aligned == FLAGS_aclint_timesync_offset) {
    // RTL: TimeSync trigger requires the low byte to be non-zero.
    if (wdata & 0xff)
      broadcastTime();
  } else if (aligned >= FLAGS_aclint_mtimecmp0_offset &&
             aligned < FLAGS_aclint_mtimecmp0_offset + uint64_t(hartCount_) * 8) {
    unsigned hartIx = (aligned - FLAGS_aclint_mtimecmp0_offset) / 8;
    timeCompare_.at(hartIx) = merge(timeCompare_.at(hartIx));
    sysmod_aclint_set_mtimecmp(hartIx, timeCompare_.at(hartIx)); // drive SV MTIP compare.
  }
}

void aclint::broadcastTime() {
  if (ctimeAddr_ == 0)
    return; // Time broadcast not configured for this instance.

  uint64_t t;
  {
    scope_guard g(g_aclint_sv_scope);
    t = sysmod_aclint_get_mtime(); // live mtime from SV.
    sysmod_aclint_pulse_timesync(); // pulse DUT cl_time_sync on broadcast.
  }

  // Issue a single 8-byte write to the core CTIME MMR. The sysmod ring master
  // is natively 64-bit, so length==8 maps to one beat with aw.size=3 and all 8
  // strobes (axi_sw_mst::a_wrapper), which is what CTime_Update requires. The
  // register is selected by the AXI address (ctimeAddr_), not the byte lane.
  std::vector<uint8_t> data_vec(8, 0);
  std::vector<bool> strb(8, true);
  for (int i = 0; i < 8; ++i, t >>= 8)
    data_vec[i] = t & 0xff;
  cvm::registry::messenger.signal(axiMstLoc_, transactor::write_request_t{ctimeAddr_, 8, data_vec, strb, false});
}
