#include <vector>
#include <cassert>
#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

class pmu
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
      DTLB_READ_ACCESS,
      DTLB_READ_MISS,
      DTLB_WRITE_ACCESS,
      DTLB_WRITE_MISS,
      DTLB_PREFETCH_ACCESS,
      DTLB_PREFETCH_MISS,
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
    } counter;

    pmu(cvm::topology::loc_t, unsigned);
    ~pmu();

    void report();

    std::vector<uint64_t> to_vector(const rv_tester_transactions::pmu::pmcounters& pmcounters)
    {
      std::vector<uint64_t> tmp(counter::COUNT);

      tmp[counter::CPU_CYCLES] = pmcounters.cpu_cycles;
      tmp[counter::INSTRUCTIONS] = pmcounters.instructions;
      tmp[counter::CACHE_REFERENCES] = pmcounters.cache_references;
      tmp[counter::CACHE_MISSES] = pmcounters.cache_misses;
      tmp[counter::BRANCH_INSTRUCTIONS] = pmcounters.branch_instructions;
      tmp[counter::BRANCH_MISSES] = pmcounters.branch_misses;
      tmp[counter::BUS_CYCLES] = pmcounters.bus_cycles;
      tmp[counter::STALLED_CYCLES_FRONTEND] = pmcounters.stalled_cycles_frontend;
      tmp[counter::STALLED_CYCLES_BACKEND] = pmcounters.stalled_cycles_backend;
      tmp[counter::REF_CPU_CYCLES] = pmcounters.ref_cpu_cycles;
      tmp[counter::L1D_READ_ACCESS] = pmcounters.l1d_read_access;
      tmp[counter::L1D_READ_MISS] = pmcounters.l1d_read_miss;
      tmp[counter::L1D_WRITE_ACCESS] = pmcounters.l1d_write_access;
      tmp[counter::L1D_WRITE_MISS] = pmcounters.l1d_write_miss;
      tmp[counter::L1D_PREFETCH_ACCESS] = pmcounters.l1d_prefetch_access;
      tmp[counter::L1D_PREFETCH_MISS] = pmcounters.l1d_prefetch_miss;
      tmp[counter::L1I_READ_ACCESS] = pmcounters.l1i_read_access;
      tmp[counter::L1I_READ_MISS] = pmcounters.l1i_read_miss;
      tmp[counter::L1I_WRITE_ACCESS] = pmcounters.l1i_write_access;
      tmp[counter::L1I_WRITE_MISS] = pmcounters.l1i_write_miss;
      tmp[counter::L1I_PREFETCH_ACCESS] = pmcounters.l1i_prefetch_access;
      tmp[counter::L1I_PREFETCH_MISS] = pmcounters.l1i_prefetch_miss;
      tmp[counter::LL_READ_ACCESS] = pmcounters.ll_read_access;
      tmp[counter::LL_READ_MISS] = pmcounters.ll_read_miss;
      tmp[counter::LL_WRITE_ACCESS] = pmcounters.ll_write_access;
      tmp[counter::LL_WRITE_MISS] = pmcounters.ll_write_miss;
      tmp[counter::LL_PREFETCH_ACCESS] = pmcounters.ll_prefetch_access;
      tmp[counter::LL_PREFETCH_MISS] = pmcounters.ll_prefetch_miss;
      tmp[counter::DTLB_READ_ACCESS] = pmcounters.dtlb_read_access;
      tmp[counter::DTLB_READ_MISS] = pmcounters.dtlb_read_miss;
      tmp[counter::DTLB_WRITE_ACCESS] = pmcounters.dtlb_write_access;
      tmp[counter::DTLB_WRITE_MISS] = pmcounters.dtlb_write_miss;
      tmp[counter::DTLB_PREFETCH_ACCESS] = pmcounters.dtlb_prefetch_access;
      tmp[counter::DTLB_PREFETCH_MISS] = pmcounters.dtlb_prefetch_miss;
      tmp[counter::ITLB_READ_ACCESS] = pmcounters.itlb_read_access;
      tmp[counter::ITLB_READ_MISS] = pmcounters.itlb_read_miss;
      tmp[counter::ITLB_WRITE_ACCESS] = pmcounters.itlb_write_access;
      tmp[counter::ITLB_WRITE_MISS] = pmcounters.itlb_write_miss;
      tmp[counter::ITLB_PREFETCH_ACCESS] = pmcounters.itlb_prefetch_access;
      tmp[counter::ITLB_PREFETCH_MISS] = pmcounters.itlb_prefetch_miss;
      tmp[counter::BPU_WRITE_ACCESS] = pmcounters.bpu_write_access;
      tmp[counter::L1D_CACHE_INVALIDATE] = pmcounters.l1d_cache_invalidate;
      tmp[counter::STALLS_MEM_L1D_MISS] = pmcounters.stalls_mem_l1d_miss;
      tmp[counter::STALLS_MEM_L1D_MISS_L2_MISS] = pmcounters.stalls_mem_l1d_miss_l2_miss;
      tmp[counter::STALLS_MEM_ANY] = pmcounters.stalls_mem_any;
      tmp[counter::STALLS_MEM_L1I_MISS] = pmcounters.stalls_mem_l1i_miss;
      tmp[counter::NFP_MISPREDICT] = pmcounters.nfp_mispredict;
      tmp[counter::BR_IMMED_SPEC] = pmcounters.br_immed_spec;
      tmp[counter::BR_INDIRECT_SPEC] = pmcounters.br_indirect_spec;
      tmp[counter::BR_RET_SPEC] = pmcounters.br_ret_spec;
      tmp[counter::LD_SPEC] = pmcounters.ld_spec;
      tmp[counter::ST_SPEC] = pmcounters.st_spec;
      tmp[counter::INT_SPEC] = pmcounters.int_spec;
      tmp[counter::FP_SPEC] = pmcounters.fp_spec;
      tmp[counter::UOP_ISSUED] = pmcounters.uop_issued;
      tmp[counter::TOTAL_UOPS_FLUSHED] = pmcounters.total_uops_flushed;
      tmp[counter::TOTAL_IND_BR_RETIRED] = pmcounters.total_ind_br_retired;
      tmp[counter::TOTAL_IND_BR_RETIRED_MISPRED] = pmcounters.total_ind_br_retired_mispred;
      tmp[counter::LSU_RESYNCS] = pmcounters.lsu_resyncs;
      tmp[counter::LOAD_MISAL_ACCESSES] = pmcounters.load_misal_accesses;
      tmp[counter::STORE_MISAL_ACCESSES] = pmcounters.store_misal_accesses;
      tmp[counter::STLF_HITS] = pmcounters.stlf_hits;
      tmp[counter::VECTOR_BUSY_CYCLES] = pmcounters.vector_busy_cycles;

      return tmp;
    }

    // this is annoying, no reflection
    const std::unordered_map<counter, std::string_view> to_string =
    {
      {CPU_CYCLES,"cpu_cycles"},
      {INSTRUCTIONS,"instructions"},
      {CACHE_REFERENCES,"cache_references"},
      {CACHE_MISSES,"cache_misses"},
      {BRANCH_INSTRUCTIONS,"branch_instructions"},
      {BRANCH_MISSES,"branch_misses"},
      {BUS_CYCLES,"bus_cycles"},
      {STALLED_CYCLES_FRONTEND,"stalled_cycles_frontend"},
      {STALLED_CYCLES_BACKEND,"stalled_cycles_backend"},
      {REF_CPU_CYCLES,"ref_cpu_cycles"},
      {L1D_READ_ACCESS,"l1d_read_access"},
      {L1D_READ_MISS,"l1d_read_miss"},
      {L1D_WRITE_ACCESS,"l1d_write_access"},
      {L1D_WRITE_MISS,"l1d_write_miss"},
      {L1D_PREFETCH_ACCESS,"l1d_prefetch_access"},
      {L1D_PREFETCH_MISS,"l1d_prefetch_miss"},
      {L1I_READ_ACCESS,"l1i_read_access"},
      {L1I_READ_MISS,"l1i_read_miss"},
      {L1I_WRITE_ACCESS,"l1i_write_access"},
      {L1I_WRITE_MISS,"l1i_write_miss"},
      {L1I_PREFETCH_ACCESS,"l1i_prefetch_access"},
      {L1I_PREFETCH_MISS,"l1i_prefetch_miss"},
      {LL_READ_ACCESS,"ll_read_access"},
      {LL_READ_MISS,"ll_read_miss"},
      {LL_WRITE_ACCESS,"ll_write_access"},
      {LL_WRITE_MISS,"ll_write_miss"},
      {LL_PREFETCH_ACCESS,"ll_prefetch_access"},
      {LL_PREFETCH_MISS,"ll_prefetch_miss"},
      {DTLB_READ_ACCESS,"dtlb_read_access"},
      {DTLB_READ_MISS,"dtlb_read_miss"},
      {DTLB_WRITE_ACCESS,"dtlb_write_access"},
      {DTLB_WRITE_MISS,"dtlb_write_miss"},
      {DTLB_PREFETCH_ACCESS,"dtlb_prefetch_access"},
      {DTLB_PREFETCH_MISS,"dtlb_prefetch_miss"},
      {ITLB_READ_ACCESS,"itlb_read_access"},
      {ITLB_READ_MISS,"itlb_read_miss"},
      {ITLB_WRITE_ACCESS,"itlb_write_access"},
      {ITLB_WRITE_MISS,"itlb_write_miss"},
      {ITLB_PREFETCH_ACCESS,"itlb_prefetch_access"},
      {ITLB_PREFETCH_MISS,"itlb_prefetch_miss"},
      {BPU_WRITE_ACCESS,"bpu_write_access"},
      {L1D_CACHE_INVALIDATE,"l1d_cache_invalidate"},
      {STALLS_MEM_L1D_MISS,"stalls_mem_l1d_miss"},
      {STALLS_MEM_L1D_MISS_L2_MISS,"stalls_mem_l1d_miss_l2_miss"},
      {STALLS_MEM_ANY,"stalls_mem_any"},
      {STALLS_MEM_L1I_MISS,"stalls_mem_l1i_miss"},
      {NFP_MISPREDICT,"nfp_mispredict"},
      {BR_IMMED_SPEC,"br_immed_spec"},
      {BR_INDIRECT_SPEC,"br_indirect_spec"},
      {BR_RET_SPEC,"br_ret_spec"},
      {LD_SPEC,"ld_spec"},
      {ST_SPEC,"st_spec"},
      {INT_SPEC,"int_spec"},
      {FP_SPEC,"fp_spec"},
      {UOP_ISSUED,"uop_issued"},
      {TOTAL_UOPS_FLUSHED,"total_uops_flushed"},
      {TOTAL_IND_BR_RETIRED,"total_ind_br_retired"},
      {TOTAL_IND_BR_RETIRED_MISPRED,"total_ind_br_retired_mispred"},
      {LSU_RESYNCS,"lsu_resyncs"},
      {LOAD_MISAL_ACCESSES,"load_misal_accesses"},
      {STORE_MISAL_ACCESSES,"store_misal_accesses"},
      {STLF_HITS,"stlf_hits"},
      {VECTOR_BUSY_CYCLES,"vector_busy_cycles"},
    };

    // snapshot current counter values, to be used in perf region
    void perf_region_start()
    {
      assert(perf_region.size() == counters.size());

      for (size_t i = 0; i < perf_region.size(); i++)
        perf_region[i] = counters[i];

      perf_region_started = true;
    }

    void perf_region_end()
    {
      assert(perf_region.size() == counters.size());

      for (size_t i = 0; i < perf_region.size(); i++) {
        perf_region[i] = counters[i] - perf_region[i];
      }

      perf_region_ended = true;
    }

    void configure();
    void process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi);
    void process(const rv_tester_transactions::pmu::pmcounters& pmcounters);

  private:

    cvm::file_logger log;
    cvm::topology::loc_t loc_;
    unsigned id_;

    std::vector<uint64_t> counters;

    uint64_t perf_start_pc;
    uint64_t perf_start_cycle = 0;
    uint64_t perf_end_pc;
    uint64_t perf_end_cycle = 0;
    bool perf_region_ok = false;
    bool perf_region_started = false;
    bool perf_region_ended = false;
    std::vector<uint64_t> perf_region;
};
