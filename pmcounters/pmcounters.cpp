#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"
#include "pmcounters.hpp"

DEFINE_bool(perf, false, "Enable core performance metrics");
DEFINE_bool(dump_pmcounters, false, "Enable pmcounters logs");

REGISTRY_register(pmcounters, TOP.PLATFORM.PMCOUNTERS, 0);

pmcounters::pmcounters(cvm::topology::loc_t loc, unsigned)
  : log("pmcounters.log")
{
  if (FLAGS_perf) {
    counters.resize(counter_t::COUNT, 0);

    if (FLAGS_dump_pmcounters) {
      for (size_t i = 0; i < to_string.size(); i++) {
        if (i != counter_t::CPU_CYCLES)
          log(cvm::NONE, ",{}", to_string[static_cast<counter_t>(i)]);
        else
          log(cvm::NONE, "{}", to_string[static_cast<counter_t>(i)]);
      }
      log(cvm::NONE, "\n");
    }

    cvm::registry::messenger.connect<pmcounter>(loc, [this](const auto& counter) { return this->pmc_update(counter); });
  }
}

pmcounters::~pmcounters()
{
  report(true);
}

void
pmcounters::report(bool final_report) {
  if (FLAGS_perf) {
    if (FLAGS_dump_pmcounters and not final_report) {
      for (size_t i = 0; i < to_string.size(); i++) {
        if (i != counter_t::CPU_CYCLES)
          log(cvm::NONE, ",{:016x}", counters[i]);
        else
          log(cvm::NONE, "{:016x}", counters[i]);
      }
      log(cvm::NONE, "\n");
    }
    else if (final_report) {
      for (size_t i = 0; i < to_string.size(); i++)
        cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}\": \"{:016x}\"}}\n", to_string[static_cast<counter_t>(i)], counters[i]);
    }
  }
}
