// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"
#include "common/memmap.h"
#include "src/sysmod/sysmod.h"
#include "sysmod_rpc.h"

// Plusargs owned by cosim and rv_tester libraries that this binary does not link
DEFINE_bool(cosim, false, "Enable cosim checking");
DEFINE_bool(rvfi, false, "Enable rvfi");
DEFINE_bool(mcm, false, "Enable mcm");
DEFINE_bool(metrics, false, "Enable printing metrics in log file");
DEFINE_bool(whisper_client_check, false, "Removing Whisper API client checks");
DEFINE_bool(time_mtime_sync_enable, false, "Enable time and mtime sync check");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(standalone, false, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(preload, false, "Whisper preload");
DEFINE_bool(cache_model_en, false, "Enable MCM Cache Model");
DEFINE_string(eot, "tohost", "Enable end-of-test mechanism");
DEFINE_uint64(tohost, 0, "Use this tohost address if provided");
DEFINE_uint64(max_instr, 0, "Max instruction limit to terminate the sim");
DEFINE_bool(eot_mem_check, false, "Do End of Test memory checks");

// Simulator entry points normally provided by the simulator: VPI plusarg access and DPI imports
extern "C" {
PLI_INT32 vpi_get_vlog_info(p_vpi_vlog_info) { return 0; }
svScope svGetScope(void) { return nullptr; }
svScope svSetScope(const svScope) { return nullptr; }
void sysmod_timer_interrupt(unsigned, unsigned, unsigned long) {}
void sysmod_sw_interrupt(unsigned, unsigned) {}
void sysmod_dmi_write(unsigned, unsigned, unsigned) {}
void sysmod_terminate() {}
void sysmod_aclint_set_mtimecmp(unsigned, unsigned long) {}
void sysmod_aclint_set_mtime(unsigned long) {}
unsigned long sysmod_aclint_get_mtime() { return 0; }
void sysmod_aclint_pulse_timesync() {}
}

namespace {

// Matches the "external0" entry of the shared testbench memmap
constexpr uint64_t kProbeBase = 0x60000000;
constexpr size_t kProbeSize = 0x1000;
constexpr const char* kMemmapPath = "test/sw_testbench/memmap.json";

class probe_device : public device {
public:
  using device::device;
  void configure() override { configured = true; }
  void tick(uint64_t advance) override { ticks += advance; }
  void jtag_tick(uint64_t advance) override { jtag_ticks += advance; }
  void overlay_tick(uint64_t advance) override { overlay_ticks += advance; }
  void is_dut_reset_req(bool, uint64_t, uint64_t) override { resets++; }

  bool configured = false;
  uint64_t ticks = 0;
  uint64_t jtag_ticks = 0;
  uint64_t overlay_ticks = 0;
  uint64_t resets = 0;
};

class SysmodAddDeviceTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { FLAGS_memmap_json_path = kMemmapPath; }

  void SetUp() override {
    loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    ASSERT_NE(loc_, cvm::topology::null);
    sysmod_ = std::make_unique<sysmod>(loc_, 0);
  }

  void TearDown() override {
    sysmod_.reset();
    cvm::registry::messenger.clear();
  }

  std::shared_ptr<probe_device> make_probe(const std::string& tag, uint64_t base = kProbeBase, size_t size = kProbeSize) {
    return std::make_shared<probe_device>(tag, base, size, loc_);
  }

  void add(std::shared_ptr<device> d) {
    cvm::registry::messenger.call<sysmod_add_device>(loc_, d);
  }

  cvm::topology::loc_t loc_;
  std::unique_ptr<sysmod> sysmod_;
};

TEST_F(SysmodAddDeviceTest, RpcRegistersDeviceForTagAndAddressLookup) {
  auto probe = make_probe("probe0");
  add(probe);

  EXPECT_TRUE(probe->configured);
  EXPECT_EQ(sysmod_->dev("probe0"), probe.get());
  EXPECT_EQ(sysmod_->dev(kProbeBase), probe.get());
  EXPECT_EQ(sysmod_->dev(kProbeBase + kProbeSize - 1), probe.get());
  EXPECT_NE(sysmod_->dev(kProbeBase + kProbeSize), probe.get());
}

TEST_F(SysmodAddDeviceTest, TickVariantsReachExternalDevice) {
  auto probe = make_probe("probe0");
  add(probe);

  sysmod_->tick(3);
  sysmod_->jtag_tick(2);
  sysmod_->overlay_tick(5);
  sysmod_->is_dut_reset_req(true, 0, 1);
  sysmod_->is_dut_reset_req(false, 0, 1);

  EXPECT_EQ(probe->ticks, 3u);
  EXPECT_EQ(probe->jtag_ticks, 2u);
  EXPECT_EQ(probe->overlay_ticks, 5u);
  EXPECT_EQ(probe->resets, 1u);
}

TEST_F(SysmodAddDeviceTest, DuplicateTagIsRejected) {
  auto first = make_probe("probe0");
  auto second = make_probe("probe0", kProbeBase + 0x100, 0x10);
  add(first);
  add(second);

  EXPECT_FALSE(second->configured);
  EXPECT_EQ(sysmod_->dev("probe0"), first.get());
  EXPECT_EQ(sysmod_->dev(kProbeBase + 0x100), first.get());
  sysmod_->tick(1);
  EXPECT_EQ(first->ticks, 1u);
  EXPECT_EQ(second->ticks, 0u);
}

TEST_F(SysmodAddDeviceTest, NullDeviceIsIgnored) {
  add(nullptr);
  sysmod_->tick(1);
  EXPECT_EQ(sysmod_->dev("probe0"), nullptr);
}

TEST_F(SysmodAddDeviceTest, TickOnlyDeviceHasNoAddressRange) {
  auto probe = make_probe("probe_tick_only", 0, 0);
  add(probe);

  sysmod_->tick(4);
  EXPECT_EQ(probe->ticks, 4u);
  EXPECT_NE(sysmod_->dev(uint64_t{0}), probe.get());
}

TEST_F(SysmodAddDeviceTest, ComposeReservesExternalRangeWithoutCreatingDevice) {
  sysmod_->compose();
  EXPECT_EQ(sysmod_->dev("external0"), nullptr);
  EXPECT_NE(sysmod_->dev("memory"), nullptr);
  EXPECT_NE(sysmod_->dev(kProbeBase), nullptr);
  EXPECT_EQ(sysmod_->dev(kProbeBase)->tag(), "fallback_null_dev");
}

TEST_F(SysmodAddDeviceTest, DeviceAddedBeforeComposeSurvivesCompose) {
  auto probe = make_probe("probe0");
  add(probe);
  sysmod_->compose();

  EXPECT_EQ(sysmod_->dev("probe0"), probe.get());
  EXPECT_EQ(sysmod_->dev(kProbeBase), probe.get());
  // Composed clint requires tick advances that are multiples of its divisor
  sysmod_->tick(100);
  EXPECT_EQ(probe->ticks, 100u);
}

TEST_F(SysmodAddDeviceTest, DeviceAddedAfterComposeIsVisible) {
  sysmod_->compose();
  auto probe = make_probe("probe0");
  add(probe);

  EXPECT_EQ(sysmod_->dev("probe0"), probe.get());
  EXPECT_EQ(sysmod_->dev(kProbeBase), probe.get());
}

} // namespace
