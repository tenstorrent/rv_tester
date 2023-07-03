#include "cvm/plusargs.hpp"
#include "pmcounters.hpp"

DEFINE_bool(perf, false, "Enable core performance metrics");
// TODO: control which are dumped? might not be useful
DEFINE_bool(dump_pmcounters, false, "Enable pmcounters logs");

pmcounters::pmcounters()
  : log("pmcounters.log")
{
  if (FLAGS_perf) {
    counters.resize(counter_t::COUNT, 0);

    if (FLAGS_dump_pmcounters) {
      for (size_t i = 0; i < to_string.size(); i++) {
        if (i != counter_t::CPU_CYCLES)
          log(cvm::NONE, ",{}", to_string.at(static_cast<counter_t>(i)));
        else
          log(cvm::NONE, "{}", to_string.at(static_cast<counter_t>(i)));
      }
      log(cvm::NONE, "\n");
    }
  }
}

pmcounters::~pmcounters()
{
  report(true);
}

void
pmcounters::report(bool final_report)
{
  if (FLAGS_perf) {
    const auto& used = (perf_region_started)? perf_region : counters;
    if (FLAGS_dump_pmcounters and not final_report) {
      for (size_t i = 0; i < to_string.size(); i++) {
        if (i != counter_t::CPU_CYCLES)
          log(cvm::NONE, ",{:x}", used[i]);
        else
          log(cvm::NONE, "{:x}", used[i]);
      }
      log(cvm::NONE, "\n");
    }
    else if (final_report) {
      for (size_t i = 0; i < to_string.size(); i++)
        cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}\": \"{:x}\"}}\n", to_string.at(static_cast<counter_t>(i)), used[i]);
    }
  }
}
