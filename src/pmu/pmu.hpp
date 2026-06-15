#include <map> 
#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"
#include "rv_tester_structs.h"
#include <cassert>
#include <vector>

class pmu {

public:

  #include "gen_events_core.hpp"
  #include "gen_events_sc.hpp"

  pmu(cvm::topology::loc_t, unsigned);
  ~pmu();

  void configure();

  void report();
  void ipc_check();
  void l1d_read_miss_check();

  // Define the enum type
  enum PerfChoice : uint32_t {
    CORE,
    SC
  };

  template <typename ContainerType, typename CycleType>
  void perf_region_start_temp(ContainerType& perf_region, const ContainerType& counters, CycleType& perf_start_cycle) {
    assert(perf_region.size() == counters.size());
  
    for (size_t i = 0; i < perf_region.size(); i++) {
      perf_region[i] = counters[i];
    }
    perf_start_cycle = counters_core[CPU_CYCLES];
  }
  
  template <typename ContainerType, typename CycleType>
  void perf_region_end_temp(ContainerType& perf_region, const ContainerType& counters, CycleType& perf_end_cycle) {
    assert(perf_region.size() == counters.size());
  
    for (size_t i = 0; i < perf_region.size(); i++) {
      perf_region[i] = counters[i] - perf_region[i];
    }
    perf_end_cycle = counters_core[CPU_CYCLES];
  }

  // snapshot current counter values, to be used in perf region
  void perf_region_start(PerfChoice perf_choice) {
    if (perf_choice == CORE) {
      perf_region_start_temp(perf_region_core, counters_core, perf_start_cycle);
    } else if ((perf_choice == SC) && (id_ == 0)) {
      perf_region_start_temp(perf_region_sc, counters_sc, perf_start_cycle_sc);
    }
  }

  void perf_region_end(PerfChoice perf_choice) {
    if (perf_choice == CORE) {
      perf_region_end_temp(perf_region_core, counters_core, perf_end_cycle);
    } else if ((perf_choice == SC) && (id_ == 0)) {
      perf_region_end_temp(perf_region_sc, counters_sc, perf_end_cycle_sc);
    }
  }

  bool is_within_range(double, double, int, bool);
  void process_core(const rv_tester_transactions::pmu_core::pmcounters_core<> &pmcounters);
  void process_sc(const rv_tester_transactions::pmu_sc::pmcounters_sc<> &pmcounters);
  void process_core(const rv_tester_transactions::pmu_core::hpmcounters_core<> &hpmcounters);
  void process_core(const rv_tester_transactions::pmu_core::pmc_checker<> &pmc_checker);
  void process(const rv_tester::terminate_called_fast &);
  std::string trigger_str_core(const rv_tester_transactions::pmu_core::pmcounters_core<> &pmcounters);
  std::string trigger_str_sc(const rv_tester_transactions::pmu_sc::pmcounters_sc<>& pmcounters);

  bool shutdown_ready();
  void get_filter_events_and_sum(uint64_t, std::vector<size_t>& , size_t&);
  size_t extract_granularity(uint64_t);
  size_t sum_event_vector(std::vector<size_t>& filtering_events);
  std::string name_event_vector(std::vector<size_t>& filtering_events);

private:

  enum SM : uint32_t {
    SYNCING,
    SYNC_UNTIL_TERMINATE,
    READY_TO_TERMINATE
  };

  std::atomic<SM> core_sm_;
  std::atomic<SM> sc_sm_;

  cvm::file_logger log_core;
  cvm::file_logger log_sc;
  cvm::topology::loc_t loc_;
  unsigned id_;

  std::array<std::uint64_t, counter_core::COUNT_CORE> counters_core;
  std::array<std::uint64_t, counter_sc::COUNT_SC> counters_sc;

  uint64_t perf_start_cycle = 0;
  uint64_t perf_end_cycle = 0;
  uint64_t perf_start_cycle_sc = 0;
  uint64_t perf_end_cycle_sc = 0;
  std::array<std::uint64_t, counter_core::COUNT_CORE> perf_region_core;
  std::array<std::uint64_t, counter_sc::COUNT_SC> perf_region_sc;

  struct event_csr_details{
    bool programmed = false;
    std::vector<size_t> event_type;
    size_t sideband_count_eventwr;
    long int event_granularity;
  };

  template <typename T>
  std::string generate_log_str(const std::map<T, std::string_view>& to_string);

  static constexpr size_t num_event_csrs = 8;
  event_csr_details event_csr_array[num_event_csrs];
  uint64_t hpmcounters_array [num_event_csrs];

  uint64_t sideband_count_terminate_ = 0;
  uint64_t expected_count_ = 0;
  uint64_t actual_count_ = 0;
  std::string event_name_ = "";
  std::unordered_map<uint64_t, size_t>::const_iterator pmc_event;
  std::unordered_map<uint64_t, std::unordered_map<uint16_t, size_t>>::const_iterator filtered_pmc_event;
};

