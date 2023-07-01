#include <vector>
#include "cvm/logger.hpp"
#include "cvm/topology.hpp"

class pmcounters
{

  public:

    typedef enum : size_t {
      CPU_CYCLES,
      INSTRUCTIONS,
      CACHE_REFERENCES,
      CACHE_MISSES,
      BRANCH_INSTRUCTIONS,
      BRANCH_MISSES,
      BUS_CYCLES,
      STALLED_CYCLES_FRONTEND,
      STALLED_CYCLES_BACKEND,
      REF_CPU_CYCLES,
      L1D_READ_ACCESS,
      L1D_READ_MISS,
      L1D_WRITE_ACCESS,
      L1D_WRITE_MISS,
      L1D_PREFETCH_ACCESS,
      L1D_PREFETCH_MISS,
      L1I_READ_ACCESS,
      L1I_READ_MISS,
      L1I_WRITE_ACCESS,
      L1I_WRITE_MISS,
      L1I_PREFETCH_ACCESS,
      L1I_PREFETCH_MISS,
      LL_READ_ACCESS,
      LL_READ_MISS,
      LL_WRITE_ACCESS,
      LL_WRITE_MISS,
      LL_PREFETCH_ACCESS,
      LL_PREFETCH_MISS,
      ITLB_READ_ACCESS,
      ITLB_READ_MISS,
      ITLB_WRITE_ACCESS,
      ITLB_WRITE_MISS,
      ITLB_PREFETCH_ACCESS,
      ITLB_PREFETCH_MISS,
      BPU_WRITE_ACCESS,
      L1D_CACHE_INVALIDATE,
      STALLS_MEM_L1D_MISS,
      STALLS_MEM_L1D_MISS_L2_MISS,
      STALLS_MEM_ANY,
      STALLS_MEM_L1I_MISS,
      NFP_MISPREDICT,
      BR_IMMED_SPEC,
      BR_INDIRECT_SPEC,
      BR_RET_SPEC,
      LD_SPEC,
      ST_SPEC,
      INT_SPEC,
      FP_SPEC,
      UOP_ISSUED,
      TOTAL_UOPS_FLUSHED,
      TOTAL_IND_BR_RETIRED,
      TOTAL_IND_BR_RETIRED_MISPRED,
      LSU_RESYNCS,
      LOAD_MISAL_ACCESSES,
      STORE_MISAL_ACCESSES,
      STLF_HITS,
      VECTOR_BUSY_CYCLES,
      COUNT
    } counter_t;

    std::unordered_map<counter_t, std::string_view> to_string =
    {
      {CPU_CYCLES, "cpu_cycles"},
      {INSTRUCTIONS, "instructions"}
    };

    struct pmcounter
    {
      counter_t name;
      uint64_t cycle;
      uint64_t value;
      bool increment = true;
    };

    pmcounters(cvm::topology::loc_t, unsigned);
    ~pmcounters();

    void report(bool final_report);
    void pmc_update(const pmcounter& counter)
    {
      auto cycle = counters[counter_t::CPU_CYCLES];
      if (counter.cycle != cycle and counter.name != counter_t::CPU_CYCLES)
        report(false);

      counters[counter.name] = (counter.increment)? counters[counter.name] + counter.value : counter.value;
      counters[counter_t::CPU_CYCLES] = counter.cycle;
    }

  private:

    cvm::file_logger log;
    std::vector<uint64_t> counters;
};
