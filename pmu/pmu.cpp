#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "pmu.hpp"
#include "sysmod/sysmod_plusargs.h"

DEFINE_bool(perf, false, "Enable core performance metrics");
// TODO: control which are dumped? might not be useful
DEFINE_uint64(sync_pmcounters_period, 0, "Sync pmcounters every X cycles. A value of 0 means no sync, only update on terminate.");
DEFINE_uint64(sync_pmcounters_instructions, 0, "Sync pmcounters every X instructions. A value of 0 means no sync, only update on terminate.");
DEFINE_bool(pmcounters_log, false, "Dump pmcounters in log");
DEFINE_bool(force_ref_clk, true, "Run with ref clk till reset completion");
DEFINE_bool(ipc_check, false, "Check IPC within a tolerance %");
DEFINE_double(ipc_expected, 0.0, "Expected IPC");
DEFINE_int32(ipc_tolerance_perc, 5, "IPC tolerance %");
DEFINE_bool(l1d_read_miss_check, false, "Check L1D miss rate within a tolerance %");
DEFINE_double(l1d_read_miss_expected, 0.0, "Expected L1D miss rate");
DEFINE_int32(l1d_read_miss_tolerance_perc, 20, "L1D miss rate tolerance %");
DEFINE_bool(pmc_sideband_check, false, "flag to toggle check with sideband counter");
DEFINE_int32(pmc_check_threshold, 5, " pmc_check_threshold %");
DEFINE_bool(ignore_pmc_reprogram, false, "toggle ignore on illegal reprograming of an event reg");

REGISTRY_register(pmu, PMCI, cvm::registry::all);

template <typename T, size_t N>
std::string pmu::generate_log_str(const std::array<std::string, N>& to_string, const T& counter_enum) {
  std::string log_str;
  log_str += fmt::format("trigger");
  for (size_t i = 0; i < N; i++) {
    log_str += fmt::format(",{}", to_string.at(static_cast<T>(i)));
  }
  log_str += fmt::format("\n");
  return log_str;
}

pmu::pmu(cvm::topology::loc_t loc, unsigned id)
  : log_core("h" + std::to_string(id) + "_pmcounters_core.log"), 
    log_sc("h" + std::to_string(id) + "_pmcounters_sc.log"), 
    loc_(loc), id_(id),
    counters_core{}, counters_sc{}, perf_region_core{}, perf_region_sc{}
{
  if (FLAGS_perf) {
    if (FLAGS_pmcounters_log != 0) {
      assert(core_to_string.size() == counter_core::COUNT_CORE);
      std::string log_str_core = generate_log_str(core_to_string, counter_core::COUNT_CORE);
      log_core(cvm::NONE, fmt::to_string(log_str_core));

      assert(sc_to_string.size() == counter_sc::COUNT_SC);
      std::string log_str_sc = generate_log_str(sc_to_string, counter_sc::COUNT_S
      log_sc(cvm::NONE, fmt::to_string(log_str_sc));
    }

    auto platform = cvm::topology::get_from_type("PLATFORM", 0);

    cvm::registry::messenger.connect<rv_tester_transactions::pmu_core::pmcounters_core<>>(loc, [this] (const auto& v) { return this->process_core(v); });
    cvm::registry::messenger.connect<rv_tester_transactions::pmu_sc::pmcounters_sc<>>(loc, [this] (const auto& v) { return this->process_sc(v); });
    cvm::registry::messenger.connect<rv_tester_transactions::pmu_core::hpmcounters_core<>>(loc, [this] (const auto& v) { return this->process_core(v); });
    cvm::registry::messenger.connect<rv_tester_transactions::pmu_core::pmc_checker<>>(loc, [this] (const auto& v) { return this->process_core(v); });
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
pmu::process_core(const rv_tester_transactions::pmu_core::pmcounters_core<>& pmcounters)
{
  if (loc_ != pmcounters.location)
      return;

  if (terminated_ and not sync_terminate_)
    return;
  else if (terminated_)
    sync_terminate_ = not pmcounters.terminate; // we need to wait until the last PMU packet

  cvm::log(cvm::HIGH, "[PMU] syncing core counters\n");

  core_to_vector(pmcounters);

  if (pmcounters.perf_start)
    perf_region_start();

  if (pmcounters.perf_end)
    perf_region_end();

  if (FLAGS_pmcounters_log != 0) {
    std::string log_str;
    log_str += fmt::format("{}", trigger_str_core(pmcounters));
    for (size_t i = 0; i < counters_core.size(); i++) {
      log_str += fmt::format(",{}", counters_core[i]);
    }
    log_str += fmt::format("\n");
    log_core(cvm::NONE, fmt::to_string(log_str));
  }
}

void
pmu::process_sc(const rv_tester_transactions::pmu_sc::pmcounters_sc<>& pmcounters)
{
  if (loc_ != pmcounters.location) 
      return;

  if (terminated_ and not sync_terminate_) 
    return;
  else if (terminated_) 
    sync_terminate_ = not pmcounters.terminate_sc; // we need to wait until the last PMU packet
  
  cvm::log(cvm::HIGH, "[PMU] syncing sc counters\n");

  sc_to_vector(pmcounters);

  if (pmcounters.perf_start_sc) 
    perf_region_start();

  if (pmcounters.perf_end_sc) 
    perf_region_end();

  if (FLAGS_pmcounters_log != 0) {
    std::string log_str;
    log_str += fmt::format("{}", trigger_str_sc(pmcounters));
    for (size_t i = 0; i < counters_sc.size(); i++) {
      log_str += fmt::format(",{}", counters_sc[i]);
    }
    log_str += fmt::format("\n");
    log_sc(cvm::NONE, fmt::to_string(log_str));
  }
}

std::string
pmu::trigger_str_core(const rv_tester_transactions::pmu_core::pmcounters_core<>& pmcounters)
{
  return pmcounters.perf_start  ? "perf_start"  :
         pmcounters.perf_end    ? "perf_end"    :
         pmcounters.terminate   ? "terminate"   :
         pmcounters.sync        ? "sync"        :
         pmcounters.overflow    ? "overflow"    :
                                  "none";
}

std::string
pmu::trigger_str_sc(const rv_tester_transactions::pmu_sc::pmcounters_sc<>& pmcounters)
{
  return pmcounters.perf_start_sc  ? "perf_start"  :
         pmcounters.perf_end_sc    ? "perf_end"    :
         pmcounters.terminate_sc   ? "terminate"   :
         pmcounters.sync_sc        ? "sync"        :
         pmcounters.overflow_sc    ? "overflow"    :
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
    std::string log_str_core;
    log_str_core += fmt::format("fast_terminate");
    for (size_t i = 0; i < counters_core.size(); i++) {
      log_str_core += fmt::format(",{}", counters_core[i]);
    }
    log_str_core += fmt::format("\n");
    log_core(cvm::NONE, fmt::to_string(log_str_core));

    if (id_ == 0) {
      std::string log_str_sc;
      log_str_sc += fmt::format("fast_terminate");
      for (size_t i = 0; i < counters_sc.size(); i++) {
        log_str_sc += fmt::format(",{}", counters_sc[i]);
      }
      log_str_sc += fmt::format("\n");
      log_sc(cvm::NONE, fmt::to_string(log_str_sc));
    }
  }
}

void
pmu::report()
{
  for (size_t i = 0; i < counter_core::COUNT_CORE; i++) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_{}\": \"{}\"}}\n", id_, core_to_string.at(static_cast<counter_core>(i)), counters_core[i]);
    if (perf_start_cycle and perf_end_cycle)
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_{}\": \"{}\"}}\n", id_, core_to_string.at(static_cast<counter_core>(i)), perf_region_core[i]);
  }
  if (id_ == 0) {
    for (size_t i = 0; i < counter_sc::COUNT_SC; i++) {
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_{}\": \"{}\"}}\n", id_, sc_to_string.at(static_cast<counter_sc>(i)), counters_sc[i]);
      if (perf_start_cycle and perf_end_cycle) {
        cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_perf_{}\": \"{}\"}}\n", id_, sc_to_string.at(static_cast<counter_sc>(i)), perf_region_sc[i]);
        }
    }
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
  const auto& used = (perf_start_cycle and perf_end_cycle)? perf_region_core : counters_core;
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
  const auto& used = (perf_start_cycle and perf_end_cycle)? perf_region_core : counters_core;
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

void
pmu::process_core(const rv_tester_transactions::pmu_core::hpmcounters_core<>& hpmcounters)
{
  hpmcounters_array[0]  = hpmcounters.hpmcounter3;
  hpmcounters_array[1]  = hpmcounters.hpmcounter4;
  hpmcounters_array[2]  = hpmcounters.hpmcounter5;
  hpmcounters_array[3]  = hpmcounters.hpmcounter6;
  hpmcounters_array[4]  = hpmcounters.hpmcounter7;
  hpmcounters_array[5]  = hpmcounters.hpmcounter8;
  hpmcounters_array[6]  = hpmcounters.hpmcounter9;
  hpmcounters_array[7]  = hpmcounters.hpmcounter10;
}
void
pmu::process_core(const rv_tester_transactions::pmu_core::pmc_checker<>& pmc_checker)
{
  if (!FLAGS_pmc_sideband_check)
    return;
  if (pmc_checker.terminate == 0){
    for (size_t i = 0; i < num_event_csrs; i++){
      if(i == pmc_checker.event_csr){
        valid_event = event_map.find(pmc_checker.event_id);
        if (valid_event == event_map.end()){
          if(!FLAGS_ignore_pmc_reprogram){
            cvm::log(cvm::ERROR, "ERROR: Hart {}: mhpmevent{} was programmed with illegal event value {:#x}\n", id_, i+3, pmc_checker.event_id);
          }
          else{
            cvm::log(cvm::NONE, "WARNING: Hart {}: mhpmevent{} was programmed with illegal event value {:#x}\n", id_, i+3, pmc_checker.event_id);
          }
        }
        else{
          event_csr_array[i].programmed = true;
          event_csr_array[i].event_type = event_map.at(pmc_checker.event_id);
          event_csr_array[i].sideband_count_eventwr  = counters_core[event_csr_array[i].event_type];
          cvm::log(cvm::HIGH, "Hart {}: mhpmevent{} was programmed with event value {:#x}\n", id_, i+3, pmc_checker.event_id);
        }
        
      }
    }
  }
  else if(pmc_checker.terminate == 1){
    for (size_t i = 0; i < num_event_csrs; i++){
      if(event_csr_array[i].programmed == true){
        sideband_count_terminate_ = counters_core[event_csr_array[i].event_type];
        expected_count_           = sideband_count_terminate_ - event_csr_array[i].sideband_count_eventwr;
        actual_count_             = hpmcounters_array[i];
        if (std::abs(static_cast<long>(expected_count_) - static_cast<long>(actual_count_)) > std::ceil(FLAGS_pmc_check_threshold * actual_count_ * 0.01) ){
          cvm::log(cvm::ERROR, "ERROR: Hart {}:  PMC hpmcount{} vs sideband mismatch for {} : expected_count:{} actual_count:{}\n", id_, i+3, core_to_string.at(static_cast<counter_core>(event_csr_array[i].event_type)), expected_count_, actual_count_);
        }
      }
    }
  }
}
