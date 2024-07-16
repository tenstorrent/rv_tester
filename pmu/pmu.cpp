#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "pmu.hpp"
#include "sysmod/sysmod_plusargs.h"

DEFINE_bool(perf, false, "Enable core performance metrics");
// TODO: control which are dumped? might not be useful
DEFINE_uint64(sync_pmcounters_period, 0, "Sync pmcounters every X cycles. A value of 0 means no sync, only update on terminate.");
DEFINE_uint64(sync_pmcounters_instructions, 0, "Sync pmcounters every X instructions. A value of 0 means no sync, only update on terminate.");
DEFINE_bool(pmcounters_log, false, "Dump pmcounters in log");
DEFINE_bool(ipc_check, false, "Check IPC within a tolerance %");
DEFINE_double(ipc_expected, 0.0, "Expected IPC");
DEFINE_int32(ipc_tolerance_perc, 5, "IPC tolerance %");
DEFINE_bool(l1d_read_miss_check, false, "Check L1D miss rate within a tolerance %");
DEFINE_double(l1d_read_miss_expected, 0.0, "Expected L1D miss rate");
DEFINE_int32(l1d_read_miss_tolerance_perc, 20, "L1D miss rate tolerance %");

REGISTRY_register(pmu, PMCI, cvm::registry::all);

pmu::pmu(cvm::topology::loc_t loc, unsigned id)
  : log("h" + std::to_string(id) + "_pmcounters.log"), loc_(loc), id_(id)
{
  if (FLAGS_perf) {
    perf_region.resize(counter::COUNT, 0);
    counters.resize(counter::COUNT, 0);

    if (FLAGS_pmcounters_log != 0) {
      std::string log_str;
      assert(to_string.size() == counter::COUNT);
      log_str += fmt::format("trigger");
      for (size_t i = 0; i < counter::COUNT; i++) {
        log_str += fmt::format(",{}", to_string.at(static_cast<counter>(i)));
      }
      log_str += fmt::format("\n");
      log(cvm::NONE, fmt::to_string(log_str));
    }

    auto platform = cvm::topology::get_from_type("PLATFORM", 0);

    cvm::registry::messenger.connect<rv_tester_transactions::pmu::pmcounters<>>(loc, [this] (const auto& v) { return this->process(v); });
    cvm::registry::messenger.connect<rv_tester::terminate_called_fast>(platform, [this] (const auto& v) { return this->process(v); });
  }
}

pmu::~pmu()
{
  if (FLAGS_perf && FLAGS_ipc_check && (FLAGS_hart_enable_mask & (1u << id_)) != 0)
      ipc_check();
  if (FLAGS_perf && FLAGS_l1d_read_miss_check && (FLAGS_hart_enable_mask & (1u << id_)) != 0)
      l1d_read_miss_check();
  if (FLAGS_perf)
      report();
}

void
pmu::process(const rv_tester_transactions::pmu::pmcounters<>& pmcounters)
{
  if (loc_ != pmcounters.location)
    return;

  if (terminated_ and not sync_terminate_)
    return;
  else if (terminated_)
    sync_terminate_ = not pmcounters.terminate; // we need to wait until the last PMU packet

  cvm::log(cvm::HIGH, "[PMU] syncing counters\n");

  counters = to_vector(pmcounters);

  if (pmcounters.perf_start)
    perf_region_start();

  if (pmcounters.perf_end)
    perf_region_end();

  if (FLAGS_pmcounters_log != 0) {
    std::string log_str;
    log_str += fmt::format("{}", trigger_str(pmcounters));
    for (size_t i = 0; i < counters.size(); i++) {
      log_str += fmt::format(",{}", counters[i]);
    }
    log_str += fmt::format("\n");
    log(cvm::NONE, fmt::to_string(log_str));
  }
}

std::string
pmu::trigger_str(const rv_tester_transactions::pmu::pmcounters<>& pmcounters)
{
  return pmcounters.perf_start  ? "perf_start"  :
         pmcounters.perf_end    ? "perf_end"    :
         pmcounters.terminate   ? "terminate"   :
         pmcounters.sync        ? "sync"        :
                                  "none";
}

void
pmu::process(const rv_tester::terminate_called_fast&)
{
  if (terminated_)
    return;

  cvm::log(cvm::HIGH, "[PMU] termination signaled, stopping further counting\n");
  terminated_ = true;
  sync_terminate_ = true;

  if (FLAGS_pmcounters_log != 0) {
    std::string log_str;
    log_str += fmt::format("fast_terminate");
    for (size_t i = 0; i < counters.size(); i++) {
      log_str += fmt::format(",{}", counters[i]);
    }
    log_str += fmt::format("\n");
    log(cvm::NONE, fmt::to_string(log_str));
  }
}

void
pmu::report()
{
  for (size_t i = 0; i < counter::COUNT; i++) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_{}\": \"{}\"}}\n", id_, to_string.at(static_cast<counter>(i)), counters[i]);
    if (perf_start_cycle and perf_end_cycle)
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_{}\": \"{}\"}}\n", id_, to_string.at(static_cast<counter>(i)), perf_region[i]);
  }

  if (perf_start_cycle and perf_end_cycle) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_start_cycle\": \"{}\"}}\n", id_, perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_end_cycle\": \"{}\"}}\n", id_, perf_end_cycle);
  }
}

bool
pmu::is_within_range(double actual, double expected, int tolerance_perc, bool higher_is_better)
{
  if (higher_is_better && actual > expected)
    return true;

  if (!higher_is_better && actual < expected)
    return true;

  double tolerance = expected * (tolerance_perc / 100.0);
  return std::abs(actual - expected) <= tolerance;
}

void
pmu::ipc_check()
{
  const auto& used = (perf_start_cycle and perf_end_cycle)? perf_region : counters;
  double ipc_actual = used[CPU_CYCLES] ? static_cast<double>(used[INSTRUCTIONS]) / static_cast<double>(used[CPU_CYCLES]) : 0.0;

  if (!is_within_range(ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc, true)) {
    cvm::log(cvm::ERROR, "Error: IPC check failed. Act: {:.2f} Exp: {:.2f} Tolerance: {} %\n", ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc);
  }
  else {
    cvm::log(cvm::NONE, "IPC check passed. Act: {:.2f} Exp: {:.2f} Tolerance: {} %\n", ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc);
  }
}

void
pmu::l1d_read_miss_check()
{
  const auto& used = (perf_start_cycle and perf_end_cycle)? perf_region : counters;
  double l1d_read_miss_actual = used[L1D_READ_ACCESS_ALL] ? static_cast<double>(used[L1D_READ_MISS]) / static_cast<double>(used[L1D_READ_ACCESS_ALL]) * 100.0 : 0.0;

  if (!is_within_range(l1d_read_miss_actual, FLAGS_l1d_read_miss_expected, FLAGS_l1d_read_miss_tolerance_perc, false)) {
    cvm::log(cvm::ERROR, "Error: l1d_read_miss check failed. Act: {:.2f} % Exp: {:.2f} % Tolerance: {} %\n", l1d_read_miss_actual, FLAGS_l1d_read_miss_expected, FLAGS_l1d_read_miss_tolerance_perc);
  }
  else {
    cvm::log(cvm::NONE, "l1d_read_miss check passed. Act: {:.2f} Exp: {:.2f} Tolerance: {} %\n", l1d_read_miss_actual, FLAGS_l1d_read_miss_expected, FLAGS_l1d_read_miss_tolerance_perc);
  }
}

bool
pmu::shutdown_ready()
{
  if (FLAGS_perf)
    {
      if (not terminated_)
        {
          cvm::log(cvm::NONE, "Warning: [PMU] asking for shutdown without termination.\n");
          // something went wrong, just allow terminate
          return true;
        }
      return terminated_ and not sync_terminate_;
    }
  else
    return true;
}
