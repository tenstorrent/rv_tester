#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "pmu.hpp"

DEFINE_bool(perf, false, "Enable core performance metrics");
// TODO: control which are dumped? might not be useful
DEFINE_uint64(sync_pmcounters_period, 0, "Sync pmcounters every X cycles. A value of 0 means no sync, only update on terminate.");
DEFINE_bool(pmcounters_log, false, "Dump pmcounters in log");
DEFINE_bool(ipc_check, false, "Check IPC within a tolerance %");
DEFINE_double(ipc_expected, 0.0, "Expected IPC");
DEFINE_int32(ipc_tolerance_perc, 5, "IPC tolerance %");
DECLARE_string(load);

REGISTRY_register(pmu, PMCI, cvm::registry::all);

pmu::pmu(cvm::topology::loc_t loc, unsigned id)
  : log("h" + std::to_string(id) + "_pmcounters.log"), loc_(loc), id_(id)
{
  if (FLAGS_perf) {
    perf_region.resize(counter::COUNT, 0);
    counters.resize(counter::COUNT, 0);

    if (FLAGS_pmcounters_log != 0) {
      assert(to_string.size() == counter::COUNT);
      for (size_t i = 0; i < counter::COUNT; i++) {
        if (i != counter::CPU_CYCLES)
          log(cvm::NONE, ",{}", to_string.at(static_cast<counter>(i)));
        else
          log(cvm::NONE, "{}", to_string.at(static_cast<counter>(i)));
      }
      log(cvm::NONE, "\n");
    }

    auto cosim = cvm::topology::get_from_type("COSIM", id_);
    auto platform = cvm::topology::get_from_type("PLATFORM", 0);

    cvm::registry::messenger.connect<rv_tester_transactions::cosim::m_rvfi<>>(cosim, [this] (const auto& v) { return this->process(v); });
    cvm::registry::messenger.connect<rv_tester_transactions::pmu::pmcounters<>>(loc, [this] (const auto& v) { return this->process(v); });
    cvm::registry::messenger.connect<rv_tester::terminate_called>(platform, [this] (const auto& v) { return this->process(v); });
  }
}

pmu::~pmu()
{
  if (FLAGS_perf && FLAGS_ipc_check)
      ipc_check();
  if (FLAGS_perf)
      report();
}

void
pmu::configure()
{
  if (FLAGS_perf and not FLAGS_load.empty()) {
    // initialize metrics
    char buffer_start[128]; char buffer_end[128];
    std::string perf_start, perf_end;
    FILE* pipe_start = popen(("nm " + FLAGS_load + " | grep __perf_start").c_str(), "r");
    FILE* pipe_end = popen(("nm " + FLAGS_load + " | grep __perf_end").c_str(), "r");
    try {
      while (fgets(buffer_start, sizeof(buffer_start), pipe_start) != NULL)
        perf_start += buffer_start;

      while (fgets(buffer_end, sizeof(buffer_end), pipe_end) != NULL)
        perf_end += buffer_end;

      int pos = perf_start.find(" ");
      perf_start_pc = std::strtoll(perf_start.substr(0, pos).c_str(), nullptr, 16);
      pos = perf_end.find(" ");
      perf_end_pc = std::strtoll(perf_end.substr(0, pos).c_str(), nullptr, 16);

      // hacky way atm
      if (perf_start != perf_end)
        perf_region_ok = true;

    } catch (...) {
      pclose(pipe_start);
      pclose(pipe_end);
      return;
    }

    pclose(pipe_start);
    pclose(pipe_end);
  }
}

void
pmu::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi)
{
  if (terminated_)
    return;

  if (perf_region_ok) {
    if (perf_start_pc == uint64_t(m_rvfi.pc_rdata))
      perf_start_cycle = m_rvfi.cycle;

    if (perf_end_pc == uint64_t(m_rvfi.pc_rdata))
      perf_end_cycle = m_rvfi.cycle;
  }
}

void
pmu::process(const rv_tester_transactions::pmu::pmcounters<>& pmcounters)
{
  if (loc_ != pmcounters.location)
    return;

  if (terminated_ and sync_terminate_)
    return;
  else
    sync_terminate_ = true;

  cvm::log(cvm::DEBUG, "[PMU] syncing counters\n");

  if (not perf_region_started and (pmcounters.cpu_cycles >= perf_start_cycle) and (perf_start_cycle != 0))
    perf_region_start();

  counters = to_vector(pmcounters);

  if (perf_region_started and not perf_region_ended and (pmcounters.cpu_cycles >= perf_end_cycle) and (perf_end_cycle != 0))
    perf_region_end();

  if (FLAGS_pmcounters_log != 0) {
    for (size_t i = 0; i < counters.size(); i++) {
      if (i != counter::CPU_CYCLES)
        log(cvm::NONE, ",{:x}", counters[i]);
      else
        log(cvm::NONE, "{:x}", counters[i]);
    }

    log(cvm::NONE, "\n");
  }
}

void
pmu::process(const rv_tester::terminate_called&)
{
  if (terminated_)
    return;

  cvm::log(cvm::HIGH, "[PMU] termination signaled, stopping further counting\n");
  terminated_ = true;
  sync_terminate_ = false;
}

void
pmu::report()
{
  const auto& used = (perf_region_started and perf_region_ended)? perf_region : counters;
  for (size_t i = 0; i < counter::COUNT; i++)
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_{}\": \"0x{:x}\"}}\n", id_, to_string.at(static_cast<counter>(i)), used[i]);

  if (perf_region_ok) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_start_pc\": \"0x{:x}\"}}\n", id_, perf_start_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_end_pc\": \"0x{:x}\"}}\n", id_, perf_end_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_start_cycle\": \"0x{:x}\"}}\n", id_, perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_end_cycle\": \"0x{:x}\"}}\n", id_, perf_end_cycle);
  }
}

bool
pmu::is_within_range(double actual, double expected, int tolerance_perc)
{
  double tolerance = expected * (tolerance_perc / 100.0);
  return std::abs(actual - expected) <= tolerance;
}

void
pmu::ipc_check()
{
  const auto& used = (perf_region_started and perf_region_ended)? perf_region : counters;
  double ipc_actual = used[CPU_CYCLES] ? static_cast<double>(used[INSTRUCTIONS]) / static_cast<double>(used[CPU_CYCLES]) : 0.0;

  if (!is_within_range(ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc)) {
    cvm::log(cvm::ERROR, "Error: IPC check failed. Act: {:.2f} Exp: {:.2f} Tolerance: {} %\n", ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc);
  }
  else {
    cvm::log(cvm::NONE, "IPC check passed. Act: {:.2f} Exp: {:.2f} Tolerance: {} %\n", ipc_actual, FLAGS_ipc_expected, FLAGS_ipc_tolerance_perc);
  }
}
