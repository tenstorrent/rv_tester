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
DEFINE_bool(aclint_ctime_per_hart, false, "Write each hart's CTIME window instead of the ring broadcast address");
DEFINE_uint64(aclint_ctime_hart_stride, 0x10000, "Per-hart CTIME window stride used with aclint_ctime_per_hart");

// Time synchronization mode and latencies (spec 7.5.2). The SV model, this
// device and the cosim bridge all derive the same values from these flags and
// the seed (aclint_cfg.h); -1 selects a random value within [0, *_max].
DEFINE_string(aclint_local_sync, "random", "ACLINT two-phase local time sync: 0 (legacy broadcast), 1 or random");
DEFINE_int64(aclint_local_sync_latency, -1, "Local sync latency Y in mtime units (-1 = random)");
DEFINE_int64(aclint_local_sync_latency_max, 65535, "Upper bound of the random local sync latency Y");
DEFINE_int64(aclint_timer_update_latency, -1, "Timer update latency Z in mtime units (-1 = random)");
DEFINE_int64(aclint_timer_update_latency_max, 255, "Upper bound of the random timer update latency Z");
DEFINE_uint64(aclint_local_sync_min_lead, 320, "Minimum Y-Z lead in mtime units for the staged CTIME write to land before the commit strobe");
DEFINE_uint64(aclint_ref_time_incr, 10, "mtime increment per reference tick; the core AclintProgRefTimeIncr is programmed to match");
DEFINE_uint64(aclint_c_mscfg1_addr, 0xbe9, "c_mscfg1 CSR address programmed by the TB to match the model (0 disables the ISS poke)");
DEFINE_bool(aclint_csr_deposit_dis, false, "Leave the core c_mscfg1 reset values in place instead of depositing the model configuration");

// -----------------------------------------------------------------------------
// SV side of the aclint model (src/sysmod/sysmod.sv). mtime, the MTIP compare,
// the reference pulse and the TimeSync commit strobe live in SV; here we only
// read/write the MMRs and start synchronizations.
// -----------------------------------------------------------------------------
extern "C" {
void sysmod_aclint_set_mtimecmp(unsigned hartid, unsigned long val);
void sysmod_aclint_set_mtime(unsigned long val);
unsigned long sysmod_aclint_get_mtime();
void sysmod_aclint_set_cfg(unsigned char local_sync, unsigned short local_sync_latency, unsigned char timer_update_latency, unsigned char ref_time_incr);
void sysmod_aclint_sync_start(unsigned long commit_mtime);
int sysmod_aclint_sync_active();
void sysmod_aclint_note_ctime_wr(unsigned long value);
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
    if (active_ && prev_)
      svSetScope(prev_);
  }
};
} // namespace

// Called once from sysmod.sv (initial block) to capture the sysmod DPI scope.
extern "C" void sysmod_aclint_register_scope() { g_aclint_sv_scope = svGetScope(); }

aclint::aclint(const std::string& tag, uint64_t addr, uint64_t size, unsigned hartCount,
               cvm::topology::loc_t loc,
               cvm::topology::loc_t axiMstLoc, uint64_t ctimeAddr)
    : device(tag, addr, size, loc, &aclint::write, &aclint::read, this), hartCount_(hartCount), soft_(hartCount),
      timeCompare_(NUM_MTIMECMP, MTIMECMP_RESET),
      axiMstLoc_(axiMstLoc), ctimeAddr_(ctimeAddr), cfg_(aclint_resolve_cfg()) {

  cvm::log(cvm::NONE, "ACLINT Model initialized with hart count: " + std::to_string(hartCount_) + "\n");
  cvm::log(cvm::NONE, "ACLINT Model config: local_sync={} local_sync_latency(Y)={} timer_update_latency(Z)={} ref_time_incr={} ctime_addr={:#x} ctime_per_hart={}\n",
           cfg_.local_sync, cfg_.local_sync_latency, cfg_.timer_update_latency, cfg_.ref_time_incr, ctimeAddr_, FLAGS_aclint_ctime_per_hart);
  {
    scope_guard g(g_aclint_sv_scope);
    sysmod_aclint_set_cfg(cfg_.local_sync, cfg_.local_sync_latency, cfg_.timer_update_latency, cfg_.ref_time_incr);
  }

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
        if (num >= NUM_MTIMECMP) {
          cvm::log(cvm::NONE, "Error: mtimecmp index " + std::to_string(num) + " out of range for " + tag + "\n");
          continue;
        }
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
             aligned < FLAGS_aclint_mtimecmp0_offset + uint64_t(NUM_MTIMECMP) * 8) {
    unsigned cmpIx = (aligned - FLAGS_aclint_mtimecmp0_offset) / 8;
    reg = timeCompare_.at(cmpIx);
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

  if (aligned == FLAGS_aclint_mtime_offset) {
    // SW mtime write: the written value is the new time base (RTL MtimeUpdate).
    uint64_t x;
    {
      scope_guard g(g_aclint_sv_scope);
      x = merge(sysmod_aclint_get_mtime());
      sysmod_aclint_set_mtime(x);
    }
    startSync(x);
  } else if (aligned == FLAGS_aclint_timesync_offset) {
    // RTL: TimeSync trigger requires the low byte to be non-zero; the cores
    // re-align to the current mtime.
    if (wdata & 0xff) {
      uint64_t x;
      {
        scope_guard g(g_aclint_sv_scope);
        x = sysmod_aclint_get_mtime();
      }
      startSync(x);
    }
  } else if (aligned >= FLAGS_aclint_mtimecmp0_offset &&
             aligned < FLAGS_aclint_mtimecmp0_offset + uint64_t(NUM_MTIMECMP) * 8) {
    unsigned cmpIx = (aligned - FLAGS_aclint_mtimecmp0_offset) / 8;
    timeCompare_.at(cmpIx) = merge(timeCompare_.at(cmpIx));
    scope_guard g(g_aclint_sv_scope);
    sysmod_aclint_set_mtimecmp(cmpIx, timeCompare_.at(cmpIx)); // drive SV MTIP compare.
  }
}

void aclint::startSync(uint64_t x) {
  if (ctimeAddr_ == 0)
    return; // Time broadcast not configured for this instance.

  if (!cfg_.local_sync) {
    // Legacy single-step path: the cores load CTIME on the write itself and
    // no commit strobe is produced (RTL MtimeBrcstEn).
    cvm::log(cvm::MEDIUM, "[ACLINT] legacy time broadcast mtime={:#x}\n", x);
    writeCtime(x);
    return;
  }

  // Two-phase local sync (spec 7.5.2): stage X+Y into the cores, commit when
  // mtime reaches X+Y-Z. A trigger while a sync is in flight is dropped, as
  // in the RTL sequencer.
  bool active;
  {
    scope_guard g(g_aclint_sv_scope);
    active = sysmod_aclint_sync_active() != 0;
  }
  if (active) {
    cvm::log(cvm::LOW, "[ACLINT] time sync trigger dropped, previous sync still in flight (mtime={:#x})\n", x);
    return;
  }
  uint64_t target = x + cfg_.local_sync_latency;
  uint64_t commit = target > cfg_.timer_update_latency ? target - cfg_.timer_update_latency : 0;
  cvm::log(cvm::MEDIUM, "[ACLINT] local time sync stage X={:#x} target X+Y={:#x} commit X+Y-Z={:#x}\n", x, target, commit);
  writeCtime(target);
  scope_guard g(g_aclint_sv_scope);
  sysmod_aclint_sync_start(commit);
}

void aclint::writeCtime(uint64_t value) {
  // Each write is a single 8-byte beat: the sysmod ring master is natively
  // 64-bit, so length==8 maps to aw.size=3 with all 8 strobes
  // (axi_sw_mst::a_wrapper), which is what CTime_Update requires. The
  // register is selected by the AXI address, not the byte lane.
  std::vector<uint8_t> data_vec(8, 0);
  std::vector<bool> strb(8, true);
  uint64_t t = value;
  for (int i = 0; i < 8; ++i, t >>= 8)
    data_vec[i] = t & 0xff;

  if (FLAGS_aclint_ctime_per_hart) {
    for (unsigned h = 0; h < hartCount_; ++h) {
      uint64_t addr = ctimeAddr_ + uint64_t(h) * FLAGS_aclint_ctime_hart_stride;
      cvm::registry::messenger.signal(axiMstLoc_, transactor::write_request_t{addr, 8, data_vec, strb, false});
    }
  } else {
    cvm::registry::messenger.signal(axiMstLoc_, transactor::write_request_t{ctimeAddr_, 8, data_vec, strb, false});
  }
  scope_guard g(g_aclint_sv_scope);
  sysmod_aclint_note_ctime_wr(value);
}
