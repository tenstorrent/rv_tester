// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Behavior-pinning tests for the bridge's CSR paths: real bridge, mocked whisper RPCs.

#include <gtest/gtest.h>

#include <cstdint>
#include <map>

#include "src/cosim/bridge/bridge.h"
#include "whisper_client.h"

// Stub for cvm's registry_dpi.o reference to the simulator-provided svGetScope.
extern "C" svScope svGetScope() {
  return nullptr;
}

// Stub for cvm's plusargs reference to the simulator-provided VPI entry point.
extern "C" int vpi_get_vlog_info(void*) {
  return 0;
}

// Flags DEFINEd in production .cpps not linked here; values mirror production defaults.
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

namespace {

using WC = whisperClient<uint64_t>;

// Whisper stand-in: per-address values and masks; pokes write through.
struct MockWhisper {
  std::map<uint64_t, uint64_t> value;
  std::map<uint64_t, uint64_t> write_mask;
  std::map<uint64_t, uint64_t> poke_mask;
  std::map<uint64_t, uint64_t> read_mask;

  uint64_t get(const std::map<uint64_t, uint64_t>& m, uint64_t addr, uint64_t dflt) const {
    auto it = m.find(addr);
    return it == m.end() ? dflt : it->second;
  }
};

MockWhisper* g_whisper = nullptr;
int g_error_count = 0;

class BridgeCsrTest : public ::testing::Test {
protected:
  void SetUp() override {
    cvm::registry::messenger.clear();
    whisper_ = MockWhisper{};
    // Baseline: whisper agrees with every spec reset; tests override per CSR.
    for (const auto& row : CSRAL::kCsrs) {
      if (row.address != CSRAL::kNoDirectAddress)
        whisper_.value[row.address] = row.reset;
    }
    g_whisper = &whisper_;
    g_error_count = 0;

    platform_ = cvm::topology::get_from_type("PLATFORM", 0);
    wloc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0);
    ASSERT_NE(platform_, cvm::topology::null);
    ASSERT_NE(wloc_, cvm::topology::null);

    // Count bridge::error_loc signals instead of scraping logs.
    cvm::registry::messenger.connect<bridge::error_loc>(platform_, [](const bridge::error_loc&) { g_error_count++; });

    cvm::registry::messenger.procedure<WC::whisperConnectRPC>(wloc_, []() -> int { return 0; });
    cvm::registry::messenger.procedure<WC::whisperConnectedRPC>(wloc_, []() -> bool { return true; });
    cvm::registry::messenger.procedure<WC::whisperPeekCsrRPC>(wloc_,
                                                              [](int, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& poke_mask, uint64_t& read_mask, bool& valid) -> bool {
                                                                value = g_whisper->get(g_whisper->value, addr, 0);
                                                                mask = g_whisper->get(g_whisper->write_mask, addr, ~0ull);
                                                                poke_mask = g_whisper->get(g_whisper->poke_mask, addr, ~0ull);
                                                                read_mask = g_whisper->get(g_whisper->read_mask, addr, ~0ull);
                                                                valid = true;
                                                                return true;
                                                              });
    cvm::registry::messenger.procedure<WC::whisperPeekRPC>(wloc_,
                                                           [](int, char resource, uint64_t addr, uint64_t& value, bool& valid) -> bool {
                                                             if (resource == 'c')
                                                               value = g_whisper->get(g_whisper->value, addr, 0);
                                                             else
                                                               value = 0;
                                                             valid = true;
                                                             return true;
                                                           });
    cvm::registry::messenger.procedure<WC::whisperPokeRPC>(wloc_,
                                                           [](int, uint64_t, char resource, uint64_t addr, uint64_t value, bool, bool, bool& valid) -> bool {
                                                             if (resource == 'c')
                                                               g_whisper->value[addr] = value;
                                                             valid = true;
                                                             return true;
                                                           });
  }

  csr_t make_csr(uint32_t addr, uint64_t wdata, uint64_t wmask) {
    return csr_t{/*valid=*/true, /*hart=*/0, /*cycle=*/100, addr, wmask, wdata};
  }

  // Runs the group-boundary CSR write check and returns how many new errors it raised.
  int check_group(bridge& b) {
    int before = g_error_count;
    rv_instr_group_t group;
    group.cycle = 100;
    b.process_dut_instr_group_retire(0, group);
    return g_error_count - before;
  }

  MockWhisper whisper_;
  cvm::topology::loc_t platform_ = 0;
  cvm::topology::loc_t wloc_ = 0;
};

TEST_F(BridgeCsrTest, CsrInitIsSelfConsistent) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  // SetUp baseline: no drift errors, mirrors identical, group check clean.
  EXPECT_EQ(g_error_count, 0);
  EXPECT_EQ(check_group(b), 0);
}

TEST_F(BridgeCsrTest, HwUpdateMatchingIssIsClean) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  // A never-seeded ISS mirror is zeros; a DUT hw update writing zeros matches.
  csr_t c = make_csr(mscratch.address, 0x0, ~0ull);
  b.process_dut_csr_hw_update(0, c);
  EXPECT_EQ(check_group(b), 0);
}

TEST_F(BridgeCsrTest, HwUpdateMismatchErrors) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  csr_t c = make_csr(mscratch.address, 0xDEAD, ~0ull);
  b.process_dut_csr_hw_update(0, c);
  EXPECT_EQ(check_group(b), 1);
}

TEST_F(BridgeCsrTest, HwUpdateIsGatedByWhisperPokeMask) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  // Bits outside whisper's poke mask are dropped from the DUT-side update.
  whisper_.poke_mask[mscratch.address] = 0xFF;
  csr_t c = make_csr(mscratch.address, 0xDE00, ~0ull);
  b.process_dut_csr_hw_update(0, c);
  EXPECT_EQ(check_group(b), 0);
}

TEST_F(BridgeCsrTest, SkipListSuppressesMismatch) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  // mip is on the resynch-skip list, so its mismatch is swallowed.
  csr_t c = make_csr(mip.address, 0x080, ~0ull);
  b.process_dut_csr_hw_update(0, c);
  EXPECT_EQ(check_group(b), 0);
}

TEST_F(BridgeCsrTest, CustomCsrCheckDisabledByDefault) {
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  // c_fecfg2 has check=false (c_* default): mismatches are never queued.
  csr_t c = make_csr(c_fecfg2.address, 0xDEAD, ~0ull);
  b.process_dut_csr_hw_update(0, c);
  EXPECT_EQ(check_group(b), 0);
}

TEST_F(BridgeCsrTest, CsrInitChecksWhisperAgainstSpecReset) {
  // Reset drift errors by default and whisper's value is adopted (mscratch: misa is exempt).
  whisper_.value[mscratch.address] = 0x123; // disagrees with the spec reset (0)
  bridge b(1, 64, 256, platform_, 0);
  b.csr_init();
  EXPECT_EQ(g_error_count, 1); // exactly the mscratch drift
  g_error_count = 0;
  // Mirrors hold whisper's value: a DUT update matching whisper is clean...
  csr_t match = make_csr(mscratch.address, 0x123, ~0ull);
  b.process_dut_csr_hw_update(0, match);
  EXPECT_EQ(check_group(b), 0);
  // ...and one matching the spec reset value instead is a mismatch.
  csr_t spec = make_csr(mscratch.address, 0x0, ~0ull);
  b.process_dut_csr_hw_update(0, spec);
  EXPECT_EQ(check_group(b), 1);
}

} // namespace
