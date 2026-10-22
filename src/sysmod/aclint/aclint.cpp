// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <sstream>
#include "src/sysmod/aclint/aclint.h"
#include "src/transactors/axi_sw/axi_sw_mst_rpc.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "svdpi.h"

DEFINE_uint64(aclint_mtime_offset, 0, "ACLINT mtime MMR offset from device base");
DEFINE_uint64(aclint_timesync_offset, 0, "ACLINT timesync MMR offset from device base (timesync1..7 follow at +8 each)");
DEFINE_uint64(aclint_mtimecmp0_offset, 0, "ACLINT mtimecmp0 MMR offset from device base (stride 8 per hart)");
DEFINE_string(aclint_scratch_offsets, "", "Comma-separated offsets of plain read/write ACLINT scratch MMRs");
DEFINE_uint64(aclint_ctime_addr, 0, "Core CTIME MMR broadcast address reaching every hart");
DEFINE_uint64(aclint_ctime_hart_addr, 0, "Hart 0 CTIME MMR window address for per-hart writes (0 disables)");
DEFINE_bool(aclint_ctime_per_hart, false, "Write each hart's CTIME window instead of the broadcast address");
DEFINE_uint64(aclint_ctime_hart_stride, 0x10000, "Per-hart CTIME window stride");

DEFINE_uint64(aclint_local_sync_latency, 0, "Local sync latency Y in mtime units (aclint_config.local_sync_latency)");
DEFINE_uint64(aclint_timer_update_latency, 0, "Timer update latency Z in mtime units (aclint_config.timer_update_latency)");
DEFINE_uint64(aclint_c_mscfg1_addr, 0, "c_mscfg1 CSR address tracked for DisAclintTimeSync and AclintProgRefTimeIncr (0 disables tracking)");

extern "C" {
void aclint_set_mtimecmp(unsigned hartid, unsigned long val);
void aclint_set_mtime(unsigned long val);
unsigned long aclint_get_mtime();
void aclint_set_ref_time_incr(unsigned char ref_time_incr);
void aclint_sync_start(unsigned long commit_mtime);
int aclint_sync_active();
void aclint_note_ctime_wr(unsigned long value);
}

namespace {
svScope g_aclint_sv_scope = nullptr;
aclint* g_aclint = nullptr;

// Enter the aclint_model scope for an SV export call, then restore.
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

// Called once from aclint.sv to capture the aclint_model scope.
extern "C" void aclint_register_scope() { g_aclint_sv_scope = svGetScope(); }

// Called from aclint.sv when the commit strobe starts.
extern "C" void aclint_sync_pulse(unsigned long mtime, unsigned long target) {
  if (g_aclint)
    g_aclint->syncPulse(mtime, target);
}

namespace {
uint16_t local_sync_latency() { return uint16_t(FLAGS_aclint_local_sync_latency > 65535 ? 65535 : FLAGS_aclint_local_sync_latency); }
uint8_t timer_update_latency() { return uint8_t(FLAGS_aclint_timer_update_latency > 255 ? 255 : FLAGS_aclint_timer_update_latency); }

std::vector<uint64_t> parse_offsets(const std::string& list) {
  std::vector<uint64_t> offsets;
  std::stringstream ss(list);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (!item.empty())
      offsets.push_back(strtoull(item.c_str(), nullptr, 0));
  }
  return offsets;
}
} // namespace

aclint::aclint(const std::string& tag, uint64_t addr, uint64_t size, unsigned hartCount,
               cvm::topology::loc_t loc,
               cvm::topology::loc_t axiMstLoc, uint64_t ctimeAddr)
    : device(tag, addr, size, loc, &aclint::write, &aclint::read, this), hartCount_(hartCount), soft_(hartCount),
      timeCompare_(NUM_MTIMECMP, MTIMECMP_RESET), timesync_(NUM_TIMESYNC_REGS, 0),
      scratchOffsets_(parse_offsets(FLAGS_aclint_scratch_offsets)), scratch_(scratchOffsets_.size(), 0),
      axiMstLoc_(axiMstLoc), ctimeAddr_(ctimeAddr), pendingSyncHarts_(NUM_TIMESYNC_REGS, 0),
      cMscfg1_(hartCount, C_MSCFG1_RESET_FIELDS) {

  g_aclint = this;
  if (axiMstLoc_ != cvm::topology::null)
    writeRespChannel_ = cvm::registry::messenger.channel<transactor::write_response_t>(axiMstLoc_);

  cvm::log(cvm::NONE, "ACLINT Model initialized with hart count: " + std::to_string(hartCount_) + "\n");
  cvm::log(cvm::NONE, "ACLINT Model config: local_sync_latency(Y)={} timer_update_latency(Z)={} ctime_addr={:#x} ctime_hart_addr={:#x} ctime_per_hart={}\n",
           local_sync_latency(), timer_update_latency(), ctimeAddr_, FLAGS_aclint_ctime_hart_addr, FLAGS_aclint_ctime_per_hart);
  if (hartCount_ > MAX_TIMESYNC_HARTS)
    cvm::log(cvm::ERROR, "Error: [ACLINT] {} harts exceed the {} timesync hart-select bits\n", hartCount_, MAX_TIMESYNC_HARTS);

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
  if (g_aclint == this)
    g_aclint = nullptr;
}

void aclint::configure() {
  device::configure();
  if (FLAGS_aclint_c_mscfg1_addr == 0)
    return;
  const unsigned nharts = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  for (unsigned i = 0; i < nharts; ++i) {
    const cvm::topology::loc_t loc = cvm::topology::get_from_type("COSIM", i);
    cvm::registry::messenger.connect<rv_tester_transactions::cosim::m_rvfi<>>(
        loc, [this](const rv_tester_transactions::cosim::m_rvfi<>& r) { return this->process(r); });
    cvm::registry::messenger.connect<rv_tester_transactions::cosim::m_csri<>>(
        loc, [this](const rv_tester_transactions::cosim::m_csri<>& c) { return this->process(c); });
  }
}

void aclint::process(const rv_tester_transactions::cosim::m_rvfi<>& rvfi) {
  if (rvfi.csr_valid && !rvfi.trap)
    trackCsrWrite(rvfi.hart, rvfi.csr_addr, rvfi.csr_wmask, rvfi.csr_wdata);
}

void aclint::process(const rv_tester_transactions::cosim::m_csri<>& csri) {
  trackCsrWrite(csri.hart, csri.addr, csri.mask, csri.data);
}

void aclint::trackCsrWrite(unsigned hart, uint64_t addr, uint64_t mask, uint64_t data) {
  if (addr != FLAGS_aclint_c_mscfg1_addr || hart >= cMscfg1_.size())
    return;
  uint64_t& shadow = cMscfg1_[hart];
  shadow = (shadow & ~mask) | (data & mask);

  const bool local_sync = ((shadow >> C_MSCFG1_DIS_TIME_SYNC_BIT) & 1) == 0;
  uint8_t incr = uint8_t((shadow >> C_MSCFG1_REF_TIME_INCR_LSB) & 0xff);
  if (incr == 0)
    incr = DEFAULT_REF_TIME_INCR;
  if (local_sync == localSync_ && incr == refTimeIncr_)
    return;

  // One model serves every hart, so the last write wins; harts programmed
  // differently cannot all be matched.
  localSync_ = local_sync;
  refTimeIncr_ = incr;
  cvm::log(cvm::LOW, "[ACLINT] hart {} c_mscfg1 write ({:#x}): local_sync={} ref_time_incr={}\n", hart, shadow, localSync_, refTimeIncr_);
  scope_guard g(g_aclint_sv_scope);
  aclint_set_ref_time_incr(refTimeIncr_);
}

aclint::hart_mask_t aclint::allHarts() const {
  hart_mask_t harts(NUM_TIMESYNC_REGS, 0);
  for (unsigned h = 0; h < hartCount_ && h < MAX_TIMESYNC_HARTS; ++h)
    harts[h / TIMESYNC_HARTS_PER_REG] |= uint64_t(1) << (h % TIMESYNC_HARTS_PER_REG);
  return harts;
}

// Bits of unpopulated harts are dropped.
aclint::hart_mask_t aclint::timesyncHarts(unsigned reg, uint64_t select) const {
  hart_mask_t harts(NUM_TIMESYNC_REGS, 0);
  harts[reg] = select & allHarts()[reg];
  return harts;
}

bool aclint::hartSelected(const hart_mask_t& harts, unsigned hart) const {
  return hart < MAX_TIMESYNC_HARTS && ((harts[hart / TIMESYNC_HARTS_PER_REG] >> (hart % TIMESYNC_HARTS_PER_REG)) & 1);
}

bool aclint::allSelected(const hart_mask_t& harts) const { return harts == allHarts(); }

unsigned aclint::selectedCount(const hart_mask_t& harts) const {
  unsigned n = 0;
  for (unsigned h = 0; h < hartCount_; ++h)
    n += hartSelected(harts, h);
  return n;
}

std::string aclint::hartList(const hart_mask_t& harts) const {
  std::string s;
  for (unsigned h = 0; h < hartCount_; ++h)
    if (hartSelected(harts, h))
      s += (s.empty() ? "" : ",") + std::to_string(h);
  return s.empty() ? "none" : s;
}

cvm::messenger::task<std::uint8_t> aclint::read(const read_t& dr, data_t& data) {
  const auto& r = dr.r;
  auto& addr = r.addr;
  auto& length = r.length;

  uint64_t byteOffset = addr - device::addr();
  uint64_t mmrRegOffset = byteOffset & ~uint64_t(0x7);
  bool upperWord = (byteOffset & 0x4) != 0;

  if (length != 4 && length != 8) {
    cvm::log(cvm::LOW, "[ACLINT] read offset {:#x} length {} unsupported: DECERR\n", byteOffset, length);
    co_return axi::RESP_DECERR;
  }

  uint64_t reg = 0;
  bool matched = false;
  if (mmrRegOffset == FLAGS_aclint_mtime_offset) {
    scope_guard g(g_aclint_sv_scope);
    reg = aclint_get_mtime(); // live mtime from SV.
    matched = true;
  } else if (mmrRegOffset >= FLAGS_aclint_timesync_offset &&
             mmrRegOffset < FLAGS_aclint_timesync_offset + uint64_t(NUM_TIMESYNC_REGS) * 8) {
    reg = timesync_.at((mmrRegOffset - FLAGS_aclint_timesync_offset) / 8);
    matched = true;
  } else if (mmrRegOffset >= FLAGS_aclint_mtimecmp0_offset &&
             mmrRegOffset < FLAGS_aclint_mtimecmp0_offset + uint64_t(NUM_MTIMECMP) * 8) {
    unsigned cmpIx = (mmrRegOffset - FLAGS_aclint_mtimecmp0_offset) / 8;
    reg = timeCompare_.at(cmpIx);
    matched = true;
  } else {
    for (size_t i = 0; i < scratchOffsets_.size(); ++i) {
      if (mmrRegOffset == scratchOffsets_[i]) {
        reg = scratch_[i];
        matched = true;
      }
    }
  }

  if (!matched) {
    cvm::log(cvm::LOW, "[ACLINT] read of unmodelled MMR offset {:#x}: DECERR\n", mmrRegOffset);
    co_return axi::RESP_DECERR;
  }

  if (length == 4)
    reg = upperWord ? (reg >> 32) : (reg & 0xffffffffULL);
  serializeInt(reg, length, data);
  co_return axi::RESP_OKAY;
}

cvm::messenger::task<std::uint8_t> aclint::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;

  uint64_t byteOffset = addr - device::addr();
  uint64_t mmrRegOffset = byteOffset & ~uint64_t(0x7);
  bool upperWord = (byteOffset & 0x4) != 0;

  if (length != 4 && length != 8) {
    cvm::log(cvm::LOW, "[ACLINT] write offset {:#x} length {} unsupported: DECERR\n", byteOffset, length);
    co_return axi::RESP_DECERR;
  }

  uint64_t wdata = 0;
  for (size_t i = 0; i < length && i < 8; ++i)
    wdata |= uint64_t(data[i]) << (i * 8);

  auto merge = [&](uint64_t old) -> uint64_t {
    if (length == 8)
      return wdata;
    if (upperWord)
      return (old & 0x00000000ffffffffULL) | ((wdata & 0xffffffffULL) << 32);
    return (old & 0xffffffff00000000ULL) | (wdata & 0xffffffffULL);
  };
  // Bits written by this access, in register position.
  const uint64_t written = (length == 8) ? wdata : (upperWord ? ((wdata & 0xffffffffULL) << 32) : (wdata & 0xffffffffULL));

  if (mmrRegOffset == FLAGS_aclint_mtime_offset) {
    // SW mtime write: the written value is the new time base (RTL MtimeUpdate).
    uint64_t x;
    {
      scope_guard g(g_aclint_sv_scope);
      x = merge(aclint_get_mtime());
      aclint_set_mtime(x);
    }
    cvm::log(cvm::MEDIUM, "[ACLINT] mtime write {:#x}: time sync requested for all harts\n", x);
    startSync(x, allHarts());
  } else if (mmrRegOffset >= FLAGS_aclint_timesync_offset &&
             mmrRegOffset < FLAGS_aclint_timesync_offset + uint64_t(NUM_TIMESYNC_REGS) * 8) {
    const unsigned reg = (mmrRegOffset - FLAGS_aclint_timesync_offset) / 8;
    timesync_.at(reg) = merge(timesync_.at(reg));
    const hart_mask_t harts = timesyncHarts(reg, written);
    const uint64_t ignored = written & ~harts[reg];
    cvm::log(cvm::MEDIUM, "[ACLINT] timesync{} write {:#x}: time sync requested for harts {}{}\n",
             reg ? std::to_string(reg) : std::string(), written, hartList(harts),
             ignored ? fmt::format(" (unpopulated hart bits {:#x} ignored)", ignored) : std::string());
    if (selectedCount(harts) != 0) {
      uint64_t x;
      {
        scope_guard g(g_aclint_sv_scope);
        x = aclint_get_mtime();
      }
      startSync(x, harts);
    }
  } else if (mmrRegOffset >= FLAGS_aclint_mtimecmp0_offset &&
             mmrRegOffset < FLAGS_aclint_mtimecmp0_offset + uint64_t(NUM_MTIMECMP) * 8) {
    unsigned cmpIx = (mmrRegOffset - FLAGS_aclint_mtimecmp0_offset) / 8;
    timeCompare_.at(cmpIx) = merge(timeCompare_.at(cmpIx));
    scope_guard g(g_aclint_sv_scope);
    aclint_set_mtimecmp(cmpIx, timeCompare_.at(cmpIx)); // drive SV MTIP compare.
  } else {
    bool matched = false;
    for (size_t i = 0; i < scratchOffsets_.size(); ++i) {
      if (mmrRegOffset == scratchOffsets_[i]) {
        scratch_[i] = merge(scratch_[i]);
        matched = true;
      }
    }
    if (!matched) {
      cvm::log(cvm::LOW, "[ACLINT] write of unmodelled MMR offset {:#x}: DECERR\n", mmrRegOffset);
      co_return axi::RESP_DECERR;
    }
  }
  co_return axi::RESP_OKAY;
}

void aclint::startSync(uint64_t x, const hart_mask_t& harts) {
  if (ctimeAddr_ == 0)
    return; // Time broadcast not configured for this instance.

  if (!localSync_) {
    // Legacy single-step path: the cores load CTIME on the write itself and
    // no commit strobe is produced (RTL MtimeBrcstEn).
    cvm::log(cvm::MEDIUM, "[ACLINT] legacy time broadcast mtime={:#x} harts {}\n", x, hartList(harts));
    writeCtime(x, harts);
    return;
  }

  // Two-phase local sync (spec 7.5.2): stage X+Y into the cores, commit when
  // mtime reaches X+Y-Z. A trigger while a sync is in flight is dropped, as
  // in the RTL sequencer.
  bool active;
  {
    scope_guard g(g_aclint_sv_scope);
    active = aclint_sync_active() != 0;
  }
  if (active) {
    cvm::log(cvm::LOW, "[ACLINT] time sync trigger for harts {} dropped, previous sync still in flight (mtime={:#x})\n", hartList(harts), x);
    return;
  }
  const uint64_t y = local_sync_latency();
  const uint64_t z = timer_update_latency();
  uint64_t target = x + y;
  uint64_t commit = target > z ? target - z : 0;
  cvm::log(cvm::MEDIUM, "[ACLINT] local time sync stage X={:#x} target X+Y={:#x} commit X+Y-Z={:#x} harts {}\n", x, target, commit, hartList(harts));
  // The commit strobe is one cluster-wide pin: every hart reloads its CTIME
  // MMR on it, so a hart left out of the staging write reloads a stale value.
  if (!allSelected(harts))
    cvm::log(cvm::LOW, "[ACLINT] partial two-phase sync: harts not staged ({} of {}) reload their previous CTIME on the commit strobe\n",
             hartCount_ - selectedCount(harts), hartCount_);
  pendingSyncHarts_ = harts;
  writeCtime(target, harts);
  scope_guard g(g_aclint_sv_scope);
  aclint_sync_start(commit);
}

void aclint::syncPulse(uint64_t mtime, uint64_t target) {
  cvm::log(cvm::MEDIUM, "[ACLINT] time sync pulse delivered at mtime={:#x}: harts {} load CTIME {:#x}\n",
           mtime, hartList(pendingSyncHarts_), target);
}

void aclint::writeCtime(uint64_t value, const hart_mask_t& harts) {
  const bool per_hart = FLAGS_aclint_ctime_per_hart || !allSelected(harts);
  if (per_hart && FLAGS_aclint_ctime_hart_addr == 0) {
    cvm::log(cvm::ERROR, "Error: [ACLINT] per-hart CTIME write needed for harts {} but aclint_ctime_hart_addr is not set\n", hartList(harts));
    return;
  }
  if (per_hart) {
    for (unsigned h = 0; h < hartCount_; ++h)
      if (hartSelected(harts, h))
        issueCtimeWrite(FLAGS_aclint_ctime_hart_addr + uint64_t(h) * FLAGS_aclint_ctime_hart_stride, value, int(h));
  } else {
    issueCtimeWrite(ctimeAddr_, value, -1);
  }
  scope_guard g(g_aclint_sv_scope);
  aclint_note_ctime_wr(value);
}

void aclint::issueCtimeWrite(uint64_t addr, uint64_t value, int hart) {
  // One 8-byte beat (aw.size=3, all strobes), as CTime_Update requires.
  std::vector<uint8_t> data_vec(8, 0);
  std::vector<bool> strb(8, true);
  uint64_t t = value;
  for (int i = 0; i < 8; ++i, t >>= 8)
    data_vec[i] = t & 0xff;
  transactor::write_request_t req{addr, 8, data_vec, strb, false};

  auto* l = +[](aclint* self, transactor::write_request_t req, uint64_t value, int hart) -> cvm::messenger::task<void> {
    axi::id_t id;
    if (!cvm::registry::messenger.call<axi_sw_mst_push_write_request_rpc>(self->axiMstLoc_, req, id)) {
      cvm::log(cvm::ERROR, "Error: [ACLINT] no AXI id for CTIME write addr={:#x}\n", req.addr);
      co_return;
    }
    self->ctimeWrOutstanding_++;
    auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(
        self->writeRespChannel_, [id](const transactor::write_response_t& wr) { return wr.id == id; });
    if (--self->ctimeWrOutstanding_ == 0)
      cvm::registry::messenger.clear_channel<transactor::write_response_t>(self->writeRespChannel_);
    const std::string target = hart < 0 ? "all harts" : "hart " + std::to_string(hart);
    if (resp.resp != axi::RESP_OKAY)
      cvm::log(cvm::ERROR, "Error: [ACLINT] CTIME write {:#x} to {} (addr={:#x}) failed with resp {}\n", value, target, req.addr, resp.resp);
    else
      cvm::log(cvm::MEDIUM, "[ACLINT] CTIME write {:#x} delivered to {} (addr={:#x})\n", value, target, req.addr);
    co_return;
  };
  cvm::registry::messenger.fork(l, this, req, value, hart);
}
