// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Unit tests for the CSRAL engine (src/cosim/csral/csral.{h,cpp}) against the
// sw_1c generation of the tables (default csr_spec.yaml + project_overrides
// + csral_defaults.yaml). No simulator, no bridge: whisper is three mocked
// RPCs on the cvm messenger, and every expectation below pins a behavior the
// CSRAL plan (docs/csral_plan.md §4.2-4.4) promises.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <gflags/gflags.h>

#include "cvm/registry.hpp"
#include "src/cosim/csral/csral.h"
#include "whisper_client.h"

// cvm's registry_dpi.o (alwayslink) references the simulator-provided
// svGetScope; there is no simulator here.
extern "C" svScope svGetScope() {
  return nullptr;
}

// cvm's plusargs .so references the simulator-provided VPI entry point; the
// tests never call cvm::plusargs::parse().
extern "C" int vpi_get_vlog_info(void*) {
  return 0;
}

// Flags DECLAREd in headers the bridge closure includes, but DEFINEd in
// production .cpps that are deliberately not linked into this test
// (rv_tester.cpp, rvfi.cpp, eot.cpp, mcmi.cpp, sysmod.cpp,
// external_interrupt_sequence.cpp). Values mirror the production defaults
// the bridge paths under test depend on.
DEFINE_bool(monitor, true, "test stub");
DEFINE_string(eot, "tohost", "test stub");
DEFINE_uint64(tohost, 0, "test stub");
DEFINE_uint64(max_instr, 0, "test stub");
DEFINE_bool(eot_mem_check, false, "test stub");
DEFINE_bool(rvfi, false, "test stub");
DEFINE_bool(cosim, false, "test stub");
DEFINE_bool(cache_model_en, false, "test stub");
DEFINE_uint64(debug_entry_pc_offset, 0x800, "test stub");
DEFINE_uint64(debug_exit_pc_offset, 0x818, "test stub");
DEFINE_uint64(debug_mem_base_offset, 0x900, "test stub");
DEFINE_uint64(debug_mem_size, 0x100, "test stub");
DEFINE_bool(mcm, false, "test stub");
DEFINE_string(memmap_json_path, "", "test stub");
DEFINE_bool(debugrom, false, "test stub");
DEFINE_string(debugrom_path, "", "test stub");
DEFINE_bool(bootrom, false, "test stub");
DEFINE_string(bootrom_path, "", "test stub");
DEFINE_bool(cplfw, false, "test stub");
DEFINE_string(cplfw_path, "", "test stub");
DEFINE_string(hex, "", "test stub");
DEFINE_string(load, "", "test stub");
DEFINE_string(load_bin, "", "test stub");
DEFINE_string(load_lz4, "", "test stub");
DEFINE_uint32(num_harts, 1, "test stub");
DEFINE_uint64(pa_mask, 0xffffffffffffffff, "test stub");
DEFINE_string(stee_secure_region, "", "test stub");
DEFINE_uint64(seed, 1, "test stub");
DEFINE_bool(io_coherency_disable, false, "test stub");
DEFINE_bool(random_imsic_intr, false, "test stub");
DEFINE_bool(time_mtime_sync_enable, true, "test stub");
// These four live in bridge.cpp, which this test deliberately does not link.
DEFINE_bool(preload, false, "test stub");
DEFINE_bool(standalone, true, "test stub");
DEFINE_bool(cov, false, "test stub");
DEFINE_string(archsample_lib_path, "", "test stub");

namespace {

using WC = whisperClient<uint64_t>;

constexpr std::uint64_t kAll = ~0ull;

// Whisper stand-in: per-address CSR values and masks. Values fall back to the
// spec reset from the CSRAL tables, so a "whisper that agrees with the spec"
// is the default and tests only state their deltas.
struct MockWhisper {
  std::map<std::uint64_t, std::uint64_t> value;
  std::map<std::uint64_t, std::uint64_t> write_mask;
  std::map<std::uint64_t, std::uint64_t> poke_mask;
  std::map<std::uint64_t, std::uint64_t> read_mask;
  std::set<std::uint64_t> peek_fail; // addresses whose peeks report invalid

  std::map<std::uint64_t, int> peeks; // whisperPeek + whisperPeekCsr, per address
  std::map<std::uint64_t, int> pokes;
  std::map<std::uint64_t, std::uint64_t> last_poke;

  std::uint64_t value_or_reset(std::uint64_t addr) const {
    auto it = value.find(addr);
    if (it != value.end())
      return it->second;
    const auto* row = CSRAL::find_by_address(addr);
    return row ? row->reset : 0;
  }

  static std::uint64_t get(const std::map<std::uint64_t, std::uint64_t>& m, std::uint64_t addr, std::uint64_t dflt) {
    auto it = m.find(addr);
    return it == m.end() ? dflt : it->second;
  }
};

MockWhisper* g_whisper = nullptr;

std::uint32_t addr_of(std::string_view name) {
  const auto* row = CSRAL::find_by_name(name);
  EXPECT_NE(row, nullptr) << name;
  return row ? row->address : 0;
}

std::uint64_t reset_of(std::string_view name) {
  const auto* row = CSRAL::find_by_name(name);
  EXPECT_NE(row, nullptr) << name;
  return row ? row->reset : 0;
}

class CsralTest : public ::testing::Test {
protected:
  void SetUp() override {
    cvm::registry::messenger.clear();
    whisper_ = MockWhisper{};
    g_whisper = &whisper_;

    // Per-test flag isolation: the constructor parses these.
    gflags::SetCommandLineOption("csral_save_restore", "misa.H");
    gflags::SetCommandLineOption("csral_reset_check", "");

    wloc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0);
    ASSERT_NE(wloc_, cvm::topology::null);

    cvm::registry::messenger.procedure<WC::whisperPeekCsrRPC>(wloc_,
                                                              [](int, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& poke_mask, uint64_t& read_mask, bool& valid) -> bool {
                                                                g_whisper->peeks[addr]++;
                                                                if (g_whisper->peek_fail.count(addr)) {
                                                                  valid = false;
                                                                  return false;
                                                                }
                                                                value = g_whisper->value_or_reset(addr);
                                                                mask = MockWhisper::get(g_whisper->write_mask, addr, kAll);
                                                                poke_mask = MockWhisper::get(g_whisper->poke_mask, addr, kAll);
                                                                read_mask = MockWhisper::get(g_whisper->read_mask, addr, kAll);
                                                                valid = true;
                                                                return true;
                                                              });
    cvm::registry::messenger.procedure<WC::whisperPeekRPC>(wloc_,
                                                           [](int, char resource, uint64_t addr, uint64_t& value, bool& valid) -> bool {
                                                             if (resource != 'c') {
                                                               value = 0;
                                                               valid = true;
                                                               return true;
                                                             }
                                                             g_whisper->peeks[addr]++;
                                                             if (g_whisper->peek_fail.count(addr)) {
                                                               valid = false;
                                                               return false;
                                                             }
                                                             value = g_whisper->value_or_reset(addr);
                                                             valid = true;
                                                             return true;
                                                           });
    cvm::registry::messenger.procedure<WC::whisperPokeRPC>(wloc_,
                                                           [](int, uint64_t, char resource, uint64_t addr, uint64_t value, bool, bool, bool& valid) -> bool {
                                                             if (resource == 'c') {
                                                               g_whisper->value[addr] = value;
                                                               g_whisper->pokes[addr]++;
                                                               g_whisper->last_poke[addr] = value;
                                                             }
                                                             valid = true;
                                                             return true;
                                                           });
  }

  std::unique_ptr<csral> make() {
    auto m = std::make_unique<csral>(1, wloc_);
    m->reset(0);
    return m;
  }

  int peeks(std::string_view name) { return whisper_.peeks[addr_of(name)]; }
  int pokes(std::string_view name) { return whisper_.pokes[addr_of(name)]; }

  MockWhisper whisper_;
  cvm::topology::loc_t wloc_ = 0;
};

// ---- reset / init ----------------------------------------------------------

TEST_F(CsralTest, ResetSeedsSpecValues) {
  auto m = make();
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("misa")), reset_of("misa"));
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("misa")), reset_of("misa"));
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("mscratch")), 0u);
}

TEST_F(CsralTest, InitCheckFlagsSpecWhisperDrift) {
  // Whisper disagrees with the spec reset for exactly one CSR.
  whisper_.value[addr_of("misa")] = 0x123;
  auto m = make();
  auto drift = m->init_check(0);
  ASSERT_EQ(drift.size(), 1u);
  EXPECT_EQ(drift[0].csr_name, "misa");
  EXPECT_EQ(drift[0].address, addr_of("misa"));
  EXPECT_EQ(drift[0].spec, reset_of("misa"));
  EXPECT_EQ(drift[0].whisper, 0x123u);
  EXPECT_EQ(m->reset_check_severity(), csral::reset_check_t::error);
  // Whisper is the run authority: both mirrors adopt its value.
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("misa")), 0x123u);
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("misa")), 0x123u);
}

// ---- DUT updates -----------------------------------------------------------

TEST_F(CsralTest, SwWriteAppliesWhisperWriteMask) {
  whisper_.write_mask[addr_of("mscratch")] = 0xFF00;
  auto m = make();
  m->sw_write(0, addr_of("mscratch"), 0xABCD, kAll, 3, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("mscratch")), 0xAB00u);
}

TEST_F(CsralTest, VlBypassesWhisperMask) {
  // Port of modify_csr_mask's vl special case: whisper's write mask does not
  // gate vl updates.
  whisper_.write_mask[addr_of("vl")] = 0x0;
  auto m = make();
  m->sw_write(0, addr_of("vl"), 0x5, kAll, 3, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("vl")), 0x5u);
}

TEST_F(CsralTest, HwUpdateGatedByPokeMask) {
  // Bits outside whisper's poke mask are dropped from csri hardware updates
  // (parity with process_dut_csr_hw_update).
  whisper_.poke_mask[addr_of("mscratch")] = 0xFF;
  auto m = make();
  m->hw_update(0, addr_of("mscratch"), 0xDE00, kAll, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("mscratch")), 0u);
  EXPECT_TRUE(m->check(0, 100).empty());
}

// ---- alias fan-out ---------------------------------------------------------

TEST_F(CsralTest, AliasFanOutCsrLevel) {
  // cycle is ALIAS_OF mcycle in the spec; updates propagate both directions.
  auto m = make();
  m->sw_write(0, addr_of("mcycle"), 0x1111, kAll, 3, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("cycle")), 0x1111u);
  m->sw_write(0, addr_of("cycle"), 0x2222, kAll, 3, 101);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("mcycle")), 0x2222u);
}

TEST_F(CsralTest, FieldAliasFanOut) {
  // fcsr[4:0] <-> fflags[4:0] and fcsr[7:5] <-> frm[2:0] (shifted views the
  // CSR-level ALIAS_OF cannot express; from csral_defaults.yaml).
  auto m = make();
  m->sw_write(0, addr_of("fflags"), 0x1F, kAll, 3, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("fcsr")) & 0x1F, 0x1Fu);
  m->sw_write(0, addr_of("frm"), 0x3, kAll, 3, 101);
  EXPECT_EQ((m->read(0, csral::src_t::dut, addr_of("fcsr")) >> 5) & 0x7, 0x3u);
  m->sw_write(0, addr_of("fcsr"), 0xEA, kAll, 3, 102);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("fflags")), 0x0Au);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("frm")), 0x7u);
}

// ---- whisper access --------------------------------------------------------

TEST_F(CsralTest, PokeWritesThroughToIssMirror) {
  auto m = make();
  int peeks_before = peeks("mstatus");
  ASSERT_TRUE(m->poke(0, addr_of("mstatus"), 0xBEEF, 100));
  EXPECT_EQ(pokes("mstatus"), 1);
  EXPECT_EQ(whisper_.last_poke[addr_of("mstatus")], 0xBEEFu);
  // The mirror was written through: reading it needs no whisper peek.
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("mstatus")), 0xBEEFu);
  EXPECT_EQ(peeks("mstatus"), peeks_before);
}

TEST_F(CsralTest, PokeFieldReadModifyWrite) {
  auto m = make();
  // Clear misa.H (bit 7) leaving every other bit of the mirror value intact.
  ASSERT_TRUE(m->poke_field(0, addr_of("misa"), "H", 0, 100));
  EXPECT_EQ(pokes("misa"), 1);
  EXPECT_EQ(whisper_.last_poke[addr_of("misa")], reset_of("misa") & ~0x80ull);
}

TEST_F(CsralTest, VolatileReadsLivePeek) {
  // mip has policy volatile: every ISS-side read peeks whisper live.
  whisper_.value[addr_of("mip")] = 0x880;
  auto m = make();
  int before = peeks("mip");
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("mip")), 0x880u);
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("mip")), 0x880u);
  EXPECT_EQ(peeks("mip"), before + 2);
  // A non-volatile CSR reads from the mirror.
  int scratch_before = peeks("mscratch");
  (void)m->read(0, csral::src_t::iss, addr_of("mscratch"));
  EXPECT_EQ(peeks("mscratch"), scratch_before);
}

TEST_F(CsralTest, MayNotExistPeeksQuietly) {
  whisper_.peek_fail.insert(addr_of("vstopei"));
  auto m = make();
  bool ok = true;
  (void)m->peek(0, addr_of("vstopei"), &ok);
  EXPECT_FALSE(ok);
}

// ---- checking --------------------------------------------------------------

TEST_F(CsralTest, CheckReportsFieldLevelMismatch) {
  auto m = make();
  m->sw_write(0, addr_of("mscratch"), 0xDEAD, kAll, 3, 100);
  auto mismatches = m->check(0, 100);
  ASSERT_EQ(mismatches.size(), 1u);
  const auto& mm = mismatches[0];
  EXPECT_EQ(mm.csr_name, "mscratch");
  EXPECT_EQ(mm.address, addr_of("mscratch"));
  EXPECT_EQ(mm.dut, 0xDEADu);
  EXPECT_EQ(mm.iss, 0u);
  EXPECT_EQ(mm.action, CSRAL::on_mismatch_t::error);
  EXPECT_EQ(mm.check_class, csral::check_class_t::sw_write);
  EXPECT_FALSE(mm.fields.empty());
  // The queue drained.
  EXPECT_TRUE(m->check(0, 101).empty());
}

TEST_F(CsralTest, PolicyActionsFollowTables) {
  auto m = make();
  // mstatus policy: skip (legacy resynch list); mip: resynch_rd (interrupt set).
  m->sw_write(0, addr_of("mstatus"), reset_of("mstatus") ^ 0x8, kAll, 3, 100);
  m->hw_update(0, addr_of("mip"), 0x880, kAll, 100);
  auto mismatches = m->check(0, 100);
  ASSERT_EQ(mismatches.size(), 2u);
  for (const auto& mm : mismatches) {
    if (mm.csr_name == "mstatus")
      EXPECT_EQ(mm.action, CSRAL::on_mismatch_t::skip);
    else if (mm.csr_name == "mip")
      EXPECT_EQ(mm.action, CSRAL::on_mismatch_t::resynch_rd);
    else
      ADD_FAILURE() << "unexpected mismatch on " << mm.csr_name;
  }
}

TEST_F(CsralTest, CheckDisabledForCustomCsr) {
  // c_fecfg2 has policy check=false (c_* default): never queued at all.
  auto m = make();
  m->hw_update(0, addr_of("c_fecfg2"), 0xDEAD, kAll, 100);
  EXPECT_TRUE(m->check(0, 100).empty());
}

TEST_F(CsralTest, NoteResynchDrainsQuietly) {
  auto m = make();
  m->sw_write(0, addr_of("mscratch"), 0xDEAD, kAll, 3, 100);
  m->note_resynch(0);
  EXPECT_TRUE(m->check(0, 100).empty());
  // One-shot: the next mismatch reports again.
  m->sw_write(0, addr_of("mscratch"), 0xBEEF, kAll, 3, 101);
  EXPECT_EQ(m->check(0, 101).size(), 1u);
}

TEST_F(CsralTest, SetCheckEnableClasses) {
  auto m = make();
  m->set_check_enable(csral::check_class_t::hw_update, false);
  m->hw_update(0, addr_of("mscratch"), 0xAA, kAll, 100);
  m->sw_write(0, addr_of("mtvec"), 0x1000, kAll, 3, 100);
  auto mismatches = m->check(0, 100);
  ASSERT_EQ(mismatches.size(), 1u);
  EXPECT_EQ(mismatches[0].csr_name, "mtvec");
}

// ---- masking conditions ------------------------------------------------------

TEST_F(CsralTest, ConditionMaskingExcludesInactiveBits) {
  constexpr std::uint64_t kMpvGva = 0x000000C000000000ull;
  auto m = make();
  ASSERT_TRUE(m->condition_active(0, "misa.H"));
  // Turn hypervisor off; drain the misa mismatch this write itself queues.
  m->sw_write(0, addr_of("misa"), reset_of("misa") & ~0x80ull, kAll, 3, 100);
  EXPECT_FALSE(m->condition_active(0, "misa.H"));
  (void)m->check(0, 100);
  // A DUT/ISS disagreement confined to H-masked bits compares clean.
  m->sw_write(0, addr_of("mstatus"), reset_of("mstatus") | kMpvGva, kAll, 3, 101);
  EXPECT_TRUE(m->check(0, 101).empty());
  // Add a live-bit disagreement: reported, and the mask excludes MPV|GVA.
  m->sw_write(0, addr_of("mstatus"), (reset_of("mstatus") | kMpvGva) ^ 0x8, kAll, 3, 102);
  auto mismatches = m->check(0, 102);
  ASSERT_EQ(mismatches.size(), 1u);
  EXPECT_EQ(mismatches[0].csr_name, "mstatus");
  EXPECT_EQ(mismatches[0].mask & kMpvGva, 0u);
  EXPECT_EQ((mismatches[0].dut ^ mismatches[0].iss) & 0x8, 0x8u);
}

TEST_F(CsralTest, SaveRestoreRoundTrip) {
  auto m = make();
  ASSERT_TRUE(m->save_restore_enabled("misa.H"));

  struct Fired {
    bool active = true;
    std::vector<std::uint32_t> restored;
    int count = 0;
  } fired;
  m->on_condition_change([&](csral::hart_id_t, const CSRAL::condition_t& c, bool now_active, const std::vector<std::uint32_t>& restored) {
    if (std::string_view(c.name) != "misa.H")
      return;
    fired.count++;
    fired.active = now_active;
    fired.restored = restored;
  });

  // Give the DUT mirror nonzero H-masked mip bits so the stash is observable.
  m->sw_write(0, addr_of("mip"), 0x1444, kAll, 3, 99);
  // Off-edge: stash captured.
  m->sw_write(0, addr_of("misa"), reset_of("misa") & ~0x80ull, kAll, 3, 100);
  EXPECT_FALSE(m->condition_active(0, "misa.H"));
  EXPECT_EQ(fired.count, 1);
  EXPECT_FALSE(fired.active);

  // Whisper loses the masked state while H is off.
  whisper_.value[addr_of("mip")] = 0;
  int mip_pokes_before = pokes("mip");

  // On-edge: the stashed masked bits are poked back into whisper.
  m->sw_write(0, addr_of("misa"), reset_of("misa"), kAll, 3, 101);
  EXPECT_TRUE(m->condition_active(0, "misa.H"));
  EXPECT_EQ(fired.count, 2);
  EXPECT_TRUE(fired.active);
  EXPECT_EQ(fired.restored.size(), 5u); // mstatus, medeleg, mideleg, mip, mie
  EXPECT_NE(std::find(fired.restored.begin(), fired.restored.end(), addr_of("mip")), fired.restored.end());
  EXPECT_GT(pokes("mip"), mip_pokes_before);
  EXPECT_EQ(whisper_.last_poke[addr_of("mip")] & 0x1444, 0x1444u);
}

TEST_F(CsralTest, SaveRestoreDisabledByEmptyPlusarg) {
  gflags::SetCommandLineOption("csral_save_restore", "");
  auto m = make();
  EXPECT_FALSE(m->save_restore_enabled("misa.H"));

  int fired = 0;
  std::vector<std::uint32_t> last_restored{1};
  m->on_condition_change([&](csral::hart_id_t, const CSRAL::condition_t& c, bool, const std::vector<std::uint32_t>& restored) {
    if (std::string_view(c.name) == "misa.H") {
      fired++;
      last_restored = restored;
    }
  });

  m->sw_write(0, addr_of("mip"), 0x1444, kAll, 3, 99);
  m->sw_write(0, addr_of("misa"), reset_of("misa") & ~0x80ull, kAll, 3, 100);
  int mip_pokes_before = pokes("mip");
  m->sw_write(0, addr_of("misa"), reset_of("misa"), kAll, 3, 101);
  // The condition still flips and the callback still fires, but nothing is
  // poked back: masking-only behavior, today's default for every condition
  // except misa.H (which this test disabled explicitly).
  EXPECT_EQ(fired, 2);
  EXPECT_TRUE(last_restored.empty());
  EXPECT_EQ(pokes("mip"), mip_pokes_before);
}

TEST_F(CsralTest, PlusargUnknownConditionThrows) {
  gflags::SetCommandLineOption("csral_save_restore", "nosuch.X");
  EXPECT_THROW(csral(1, wloc_), std::invalid_argument);
}

TEST_F(CsralTest, PlusargViewOnlyConditionThrows) {
  // mideleg.SSIP gates only the sip delegation view: mask-only by
  // construction, save/restore is meaningless.
  gflags::SetCommandLineOption("csral_save_restore", "misa.H,mideleg.SSIP");
  EXPECT_THROW(csral(1, wloc_), std::invalid_argument);
}

// ---- default legalize hooks ---------------------------------------------------

TEST_F(CsralTest, PmpGranularityHookKeysOnNapot) {
  // Port of modify_csr_data (bridge.cpp:3216): the low-bit read-back
  // adjustment is PMP granularity behavior keyed on pmpcfg A[1] (NAPOT bit 4
  // of the entry's byte) — NOT the lock bit. A NAPOT entry reads its low 9
  // bits as ones; TOR/OFF reads its low 10 bits as zeros, lock state
  // regardless (the lock bit only gates ISS-side updates).
  whisper_.value[addr_of("pmpcfg0")] = 0x18; // A=NAPOT, unlocked
  auto m = make();
  m->sw_write(0, addr_of("pmpaddr0"), 0x12345, kAll, 3, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("pmpaddr0")), 0x12345ull | 0x1FF);

  whisper_.value[addr_of("pmpcfg0")] = 0x80; // locked, A=OFF
  m->sw_write(0, addr_of("pmpaddr0"), 0x12345, kAll, 3, 101);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("pmpaddr0")), 0x12345ull & ~0x3FFull);
}

TEST_F(CsralTest, StateenHierarchyHookMasksByMstateen) {
  // Port of modify_csr_data: sstateen0 writes are ANDed with mstateen0.
  whisper_.value[addr_of("mstateen0")] = 0x1;
  auto m = make();
  m->sw_write(0, addr_of("sstateen0"), 0x3, kAll, 1, 100);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("sstateen0")), 0x1u);
}

// ---- ISS updates and reporting -------------------------------------------------

TEST_F(CsralTest, IssWriteUpdatesIssMirrorOnly) {
  auto m = make();
  m->iss_write(0, addr_of("mepc"), 0x8000, 100);
  EXPECT_EQ(m->read(0, csral::src_t::iss, addr_of("mepc")), 0x8000u);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("mepc")), 0u);
}

TEST_F(CsralTest, HypervisorCsrsGatedByExistsIf) {
  // Whole-CSR existence gating (bridge.h hypervisor_csr_map_ parity via the
  // exists_if policy): while misa.H is 0, hstatus does not exist — no mirror
  // updates, no checks, and write_mask returns 0 without peeking whisper.
  auto m = make();
  ASSERT_TRUE(m->condition_active(0, "misa.H")); // spec reset has H=1
  m->sw_write(0, addr_of("misa"), reset_of("misa") & ~0x80ull, kAll, 3, 100);
  ASSERT_FALSE(m->condition_active(0, "misa.H"));
  (void)m->check(0, 100); // drain the (legitimate) misa mismatch this queued

  const std::uint64_t hstatus_before = m->read(0, csral::src_t::dut, addr_of("hstatus"));
  m->sw_write(0, addr_of("hstatus"), 0xDEAD, kAll, 3, 101);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("hstatus")), hstatus_before);
  EXPECT_EQ(m->write_mask(0, addr_of("hstatus")), 0u);
  m->iss_write(0, addr_of("hstatus"), 0xBEEF, 102);
  EXPECT_TRUE(m->check(0, 103).empty());

  // With H back on, hstatus exists and updates flow again.
  m->sw_write(0, addr_of("misa"), reset_of("misa"), kAll, 3, 104);
  ASSERT_TRUE(m->condition_active(0, "misa.H"));
  m->sw_write(0, addr_of("hstatus"), 0xDEAD, kAll, 3, 105);
  EXPECT_EQ(m->read(0, csral::src_t::dut, addr_of("hstatus")), 0xDEADull);
}

TEST_F(CsralTest, DumpEmitsAllCsrs) {
  auto m = make();
  std::size_t count = 0;
  m->dump(0, [&](std::string_view, std::uint64_t, std::uint64_t) { count++; });
  EXPECT_EQ(count, CSRAL::kNumCsrs);
}

} // namespace
