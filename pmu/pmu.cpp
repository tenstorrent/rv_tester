#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "pmu.hpp"

static bool validate_period(const char* flagname, uint64_t value) {
  if (value == 0) {
    cvm::log(cvm::NONE, "Invalid value for +{}={}\n", flagname, value);
    return false;
  }
  return true;
}

DEFINE_bool(perf, false, "Enable core performance metrics");
// TODO: control which are dumped? might not be useful
DEFINE_uint64(pmcounters_period, 10000, "Update pmcounters every X cycles");
DEFINE_validator(pmcounters_period, &validate_period);
DEFINE_bool(pmcounters_log, false, "Dump pmcounters in log");
DECLARE_string(load);

REGISTRY_register(pmu, TOP.PLATFORM.PMU, 0);

pmu::pmu(cvm::topology::loc_t loc, unsigned)
  : log("pmcounters.log")
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

    auto cosim = cvm::topology::get_from_type("COSIM", 0);

    cvm::registry::messenger.connect<rv_tester_transactions::cosim::m_rvfi>(cosim, [this] (const auto& v) { return this->process(v); });
    cvm::registry::messenger.connect<rv_tester_transactions::pmu::pmcounters>(loc, [this] (const auto& v) { return this->process(v); });
  }
}

pmu::~pmu()
{
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
pmu::process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi)
{
  if (perf_region_ok) {
    if (perf_start_pc == uint64_t(m_rvfi.pc_rdata))
      perf_start_cycle = m_rvfi.cycle;

    if (perf_end_pc == uint64_t(m_rvfi.pc_rdata))
      perf_end_cycle = m_rvfi.cycle;
  }
}

void
pmu::process(const rv_tester_transactions::pmu::pmcounters& pmcounters)
{
  if (pmcounters.cpu_cycles >= perf_start_cycle)
    perf_region_start();

  counters = to_vector(pmcounters);

  if (pmcounters.cpu_cycles >= perf_end_cycle)
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
pmu::report()
{
  const auto& used = (perf_region_started and perf_region_ended)? perf_region : counters;
  for (size_t i = 0; i < counter::COUNT; i++)
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}\": \"0x{:x}\"}}\n", to_string.at(static_cast<counter>(i)), used[i]);

  if (perf_region_ok) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_start_pc\": \"0x{:x}\"}}\n", perf_start_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_end_pc\": \"0x{:x}\"}}\n", perf_end_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_start_cycle\": \"0x{:x}\"}}\n", perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_end_cycle\": \"0x{:x}\"}}\n", perf_end_cycle);
  }
}
