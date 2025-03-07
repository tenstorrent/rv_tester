#include <iostream>
#include <thread>
#include <unordered_map>
#include <set>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <fmt/ranges.h>
#include "cvm/random.hpp"
#include "sysmod.h"
#include "mem/sysmod_mem.h"
#include "clint/clint.h"
#include "aclint/aclint.h"
#include "dm/dm.h"
#include "trace_cfg/trace_cfg.h"
#include "io_dev/io_dev.h"
#include "null_dev/null_dev.h"
#include "heartbeat/heartbeat.h"
#include "htif/htif.h"
#include "aplic/aplic_device.h"
#include "Uart8250.hpp"
#include "io_device.h"
#include "trickbox/trickbox.h"
#include "rv_tester/rv_tester_structs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "cosim/bridge_if/bridge_params.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "pmu/pmu_plusargs.h"
#include "cosim/utils/general/util.h"
#include "sysmod_params.hpp"
#include "cosim/bridge/bridge_plusargs.h"
#include "rv_tester_transactions.hpp"

// internal flags
DEFINE_uint64(seed, 1, "Simulation seed passed down for randomization");
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(load_lz4, "", "lz4 compressed file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_string(load_bin, "", "Binary file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_bool(bootrom, true, "Load bootrom before test");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_bool(cplfw, false, "Load cpl firmware before test");
DEFINE_string(cplfw_path, "", "Path to cpl firmware object file");
DEFINE_string(load_io, "", "load specified io dev with content from memory");
DEFINE_bool(sysmod_tick_async, true, "Asynchronous sysmod_tick calls");
DEFINE_uint64(sysmod_tick_update_threshold, 1, "Slow down tick update frequency by this factor. The tick is still eventually advanced the same cumulative amount, just not as often. Useful for emulation where the clock counts much faster but tests setup interrupts to happen very soon for simulation. They git hit by an interrupt storm and are stuck in the interrupt handler forever.");
DEFINE_string(set_csr, "", "+set_csr=<csr_num>:<value>,<num2>:<val2> ");
DEFINE_string(set_mmr, "", "+set_mmr=<addr>:<size>:<value>,<addr2>:<size>:<val2>");
//core harvesting
DEFINE_bool(rand_core_harvest, false, "Randomize core harvest options");
DEFINE_uint32(num_harts, 0, "Number of enabled harts - upto 8");
DEFINE_uint32(hart_enable_mask, 0, "Hart enable mask. Ex: With 2 enabled harts in a 8-hart system, could ie 0x18. Should match num_harts.");
DEFINE_string(hart_enable_id, "", "Hart id sequence corresponding to physical cores. Ex: With 2 enabled harts in a 8-hart system, could be 4,3 i.e. hart0=core4, hart1=core3.");
// SC harvesting
DEFINE_bool(rand_sc_harvest, false, "Randomize sc harvest options");
DEFINE_int32(num_sc_dis_ways, -1, "Number of disabled SC ways - upto 24 in multiples of 4");
DEFINE_int32(sc_dis_ways_mask, -1, "SC way enable mask. Ex: With 20 enabled ways out of 24, could be 0xF0_FFFF.");
// scratchpad 
DEFINE_bool(enable_sp_init, false, "Enable sharedcache scratchpad initilization from bootrom");
DEFINE_bool(rand_sp_ways, false, "Randomize number of SC ways reserved for scratchpad");
DEFINE_int32(num_sp_ways, -1, "Number of SC ways reserved for scratchpad");

DEFINE_uint32(trace_enable, 1, "Trace enable fuse");
DEFINE_int32(strobe_type,4, "strobe type need to be driven for random access");
DEFINE_uint32(overlay_num_times,3, "Maximum number of debug snippets to be driven");
DEFINE_int32(overlay_idle,5, "Number of idle cycles between each transfer");
DEFINE_int32(start_overlay_access,10, "Start tick point for starting overlay access");
DEFINE_uint32(debug_enable, 3, "Debug enable fuse");
DEFINE_bool(hart_sync_en, true, "Enable hart sync routine in bootrom");
DEFINE_bool(export_control_en, false, "Enable export control to reduce FP double precision");
DEFINE_uint32(mem_manager_page_size, 4096, "Mem manager internal page size");
// STEE
DEFINE_string(stee_secure_region, "", "colon separated pair of number (same as whisper's --steesr)");
DEFINE_uint32(matp_swid, 0, "MATP.SWID");
DEFINE_uint64(pa_mask, 0x0080000000000000, "address bit(s) that act as STEE distinction");
DEFINE_bool(sysmod_terminate, true, "Set to false for offline DPI mode");

REGISTRY_register(sysmod, TOP.PLATFORM.SYSMOD, 0);
// APLIC
DEFINE_uint32(aplic_sources, 33, "Number of APLIC interrupt sources");
// Uart8250
DEFINE_uint32(uart8250_iid, 1, "Interrupt identity of the uart8250 device");

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val, unsigned long mtime_val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_trace_info(unsigned trace_info_s);
  void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val);
  void sysmod_jtag_req(unsigned cmd,unsigned long upper_val, unsigned long lower_val, unsigned length, unsigned quit,unsigned tap_cfg_sel);
  void sysmod_terminate();
}

sysmod::sysmod(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id)
{
  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<uint64_t>(
      loc_,
      [this](const uint64_t&) {
      this->load_csr_mmr_boot(0);
      this->store_dm_rand();
      cosim_init_ = true;
      return;
      });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->is_dut_reset_req(t.dut_reset_req,t.clocks,t.divisor); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::jtag_tick<>& t) { return this->jtag_tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::overlay_tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::overlay_tick<>& t) { return this->overlay_tick(t.advance); });
  //cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_rdata<>>(
  //    loc_,
  //    [this](const rv_tester_transactions::sysmod::jtag_rdata<>& t) { return this->jtag_resp(t.rdata); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::event_triggers<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::event_triggers<>& t) { return this->tboxtrig_updatemem(t.addr,t.data); });
  cvm::registry::messenger.connect<sysmod::backdoor_read_t>(
      loc_,
      [this](sysmod::backdoor_read_t t) {
      auto *task = +[] (sysmod* m, backdoor_read_t w) -> cvm::messenger::task<void> {
          *w.out_data = co_await m->backdoor_read(w.address);
          *w.flag = true;
          w.flag->notify_one();
          co_return;
      };
      cvm::registry::messenger.fork(task, this, std::move(t));
      }
      );

  uint32_t num_harts = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  for(uint32_t i = 0 ; i < num_harts ; i++) {
    int unsigned location = cvm::topology::get_from_type("CORE", i);
    cvm::registry::messenger.connect<inval_load_s>(location , [this] (const auto& payload) { return this->store_inval_load(payload); });
    cvm::registry::messenger.connect<inval_crsp_s>(location , [this] (const auto& payld) { return this->store_inval_crsp(payld, 1 /*mcm*/); });
  }

  cvm::registry::messenger.connect<cbo_inval_nomcm_s>(loc_, [this] (const auto& cbo_inval_nomcm) {
    inval_crsp_s payload;
    payload.address = cbo_inval_nomcm.address;
    return this->store_inval_crsp(payload, 0/*nomcm*/);
  });
  cvm::registry::messenger.connect<sysmod::backdoor_write_t>(
      loc_,
      [this](sysmod::backdoor_write_t t) { 
      auto *task = +[] (sysmod* m, backdoor_write_t w) -> cvm::messenger::task<void> {
          co_await m->backdoor_write(w);
          *w.flag = true;
          w.flag->notify_one();
          co_return;
      };
      cvm::registry::messenger.fork(task, this, std::move(t));
      }
      );


  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");
    for (const auto& source : sources) {
        cvm::registry::messenger.connect<transactor::write_t>(
            source,
            [this, source](const auto& w) {
                if (this->dev(w.addr)) {
                    cvm::log(cvm::HIGH, "[sysmod] write: src={} addr={:#x}\n", source, w.addr);
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {w});
                }
        });
        cvm::registry::messenger.connect<transactor::read_t>(
            source,
            [this, source](const auto& r) {
                if (this->dev(r.addr)){
                    cvm::log(cvm::HIGH, "[sysmod] read: src={} id={}, addr={:#x}, len={}\n", source, r.id, r.addr, r.length);
                    cvm::registry::messenger.signal<device::read_t>(this->loc_, {{r.id, r.addr & ~FLAGS_pa_mask, r.length}, source});
                }
        });
  }
}

void sysmod::configure()
{
  if (FLAGS_cosim)
    cosim_init_ = false; // this will be set once bridge is init
  else
    cosim_init_ = true;
  // Flags configuration
  core_harvest_plusargs();
  sc_harvest_plusargs();
  // Reset configuration
  reset();
}

void
sysmod::core_harvest_plusargs()
{
  // Plusargs: num_harts, hart_enable_mask, hart_enable_id
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  uint32_t nharts = FLAGS_num_harts;
  uint32_t mask = FLAGS_hart_enable_mask;
  uint32_t mask_harts = std::bitset<32>(mask).count();
  std::vector<uint32_t> id{};
  uint32_t id_mask = 0;
  std::istringstream ss(FLAGS_hart_enable_id);
  std::string token;
  while (std::getline(ss, token, ',')) {
    if (token != "") {
      uint32_t t = std::stoull(token);
      id.push_back(t);
      id_mask |= (1 << t);
    }
  }
  uint32_t id_harts = std::bitset<32>(id_mask).count();

  // Basic validation of plusargs
  if ((nharts != 0) && (nharts > ncores))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: +num_harts {} should be between [1,{}]\n", nharts, ncores);

  if ((mask_harts != 0) && (mask_harts > ncores))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: count(+hart_enable_mask {:#x}) should be between [1,{}]\n", mask, ncores);

  if ((id_harts != 0) && (id_harts > ncores))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: count(+hart_enable_id {}) should be between [1,{}]\n", id_harts, ncores);

  // switch {num_harts, hart_enable_mask, hart_enable_id}
  uint8_t expr = ((nharts != 0) << 2) | ((mask != 0) << 1) | !id.empty();
  switch (expr) {
    case 0: // Nothing specified. Assume nharts = ncores.
      FLAGS_num_harts = ncores;
      FLAGS_hart_enable_mask = (1u << ncores) - 1;
      if (FLAGS_rand_core_harvest)
        FLAGS_hart_enable_id = get_rand_id(FLAGS_hart_enable_mask, ncores);
      else
        FLAGS_hart_enable_id = get_id(FLAGS_hart_enable_mask, ncores);
      break;
    case 1: // +hart_enable_id
      FLAGS_num_harts = id_harts;
      FLAGS_hart_enable_mask = id_mask;
      break;
    case 2: // +hart_enable_mask
      FLAGS_num_harts = mask_harts;
      if (FLAGS_rand_core_harvest)
        FLAGS_hart_enable_id = get_rand_id(mask, ncores);
      else
        FLAGS_hart_enable_id = get_id(mask, ncores);
      break;
    case 3: // +hart_enable_mask, +hart_enable_id
      if (mask_harts != id_harts)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: count(+hart_enable_mask {:#x}) != count(+hart_enable_id {})\n", mask, id);
      FLAGS_num_harts = mask_harts;
      break;
    case 4: // +num_harts
      if (FLAGS_rand_core_harvest) {
        FLAGS_hart_enable_mask = get_rand_mask(nharts, ncores);
        FLAGS_hart_enable_id = get_rand_id(FLAGS_hart_enable_mask, ncores);
      } else {
        FLAGS_hart_enable_mask = (1u << nharts) - 1;
        FLAGS_hart_enable_id = get_id(FLAGS_hart_enable_mask, ncores);
      }
      break;
    case 5: // +num_harts, +hart_enable_id
      if (nharts != id_harts)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: (+num_harts {}) != count(+hart_enable_id {})\n", nharts, id);
      FLAGS_hart_enable_mask = id_mask;
      break;
    case 6: // +num_harts, +hart_enable_mask
      if (nharts != mask_harts)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: (+num_harts {}) != count(+hart_enable_mask {:#x})\n", nharts, mask);
      FLAGS_hart_enable_id = get_rand_id(mask, ncores);
      break;
    case 7: // +num_harts, +hart_enable_mask, +hart_enable_id
      if ((nharts != mask_harts) || (nharts != id_harts))
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: (+num_harts {}) != count(+hart_enable_mask {:#x}) != count(+hart_enable_id {})\n", nharts, mask, id);
      if (mask != id_mask)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: (+hart_enable_mask {:#x}) != mask(+hart_enable_id {}\n", mask, id);
      break;
  }
  cvm::log(cvm::NONE, "[plusargs] +num_harts {} +hart_enable_mask {:#x} +hart_enable_id {}\n",
    FLAGS_num_harts, FLAGS_hart_enable_mask, FLAGS_hart_enable_id);
}

void
sysmod::sc_harvest_plusargs()
{

  FLAGS_rand_sp_ways = FLAGS_rand_sp_ways || FLAGS_enable_sp_init;
  // Plusargs: num_sc_dis_ways, sc_dis_ways_mask, num_sp_ways
  int32_t nways = cvm::topology::attr(cvm::topology::get_from_type("CORE", 0), "SC_NUM_WAYS").second;
  int32_t dis_ways = FLAGS_num_sc_dis_ways;
  int32_t sp_ways = FLAGS_num_sp_ways;
  int32_t mask = FLAGS_sc_dis_ways_mask;
  int32_t mask_dis_ways = (int32_t)std::bitset<32>(mask).count();

  // Basic validation of plusargs
  if (nways > 32)
    cvm::log(cvm::ERROR, "Error: Invalid topology attr: SC_NUM_WAYS {}, should be between [0,32]\n", nways);

  if ((dis_ways != -1) && (dis_ways < 0 || dis_ways > nways))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: +num_sc_dis_ways {}, should be between [0,{}]\n", dis_ways, nways);

  if ((dis_ways != -1) && (dis_ways % 4 != 0))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: +num_sc_dis_ways {}, should be a multiple of 4\n", dis_ways, nways);

  if ((sp_ways != -1) && (sp_ways < 0 || sp_ways > nways))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: +num_sp_ways {}, should be between [0,{}]\n", sp_ways);

  if ((mask != -1) && (mask_dis_ways < 0 || mask_dis_ways > nways))
    cvm::log(cvm::ERROR, "Error: Invalid plusarg: count(+sc_dis_ways_mask {}) should be between [0,{}]\n", mask, nways);

  // switch {num_sc_dis_ways, sc_dis_ways_mask, num_sp_ways}
  uint8_t expr = ((dis_ways != -1) << 2) | ((mask != -1) << 1) | (sp_ways != -1);
  switch (expr) {
    case 0:
      FLAGS_num_sc_dis_ways  = 0;
      FLAGS_sc_dis_ways_mask = 0;
      FLAGS_num_sp_ways      = 0;
      if (FLAGS_rand_sc_harvest) {
        FLAGS_num_sc_dis_ways = get_rand_dis_ways(nways);
        FLAGS_sc_dis_ways_mask = get_rand_ways_mask(FLAGS_num_sc_dis_ways, nways);
        cvm::log(cvm::MEDIUM, " Randomizing sc ways : {}, num_sc_dis_ways = {}\n", nways, FLAGS_num_sc_dis_ways );
      }
      if (FLAGS_rand_sp_ways)
        FLAGS_num_sp_ways = get_rand_sp_ways(nways - FLAGS_num_sc_dis_ways);
      break;

    case 1: // +num_sp_ways
      FLAGS_num_sc_dis_ways = 0;
      FLAGS_sc_dis_ways_mask = 0;
      if (FLAGS_rand_sc_harvest) {
        FLAGS_num_sc_dis_ways = get_rand_dis_ways(nways - sp_ways);
        FLAGS_sc_dis_ways_mask = get_rand_ways_mask(FLAGS_num_sc_dis_ways, nways);
      }
      break;

    case 2: // +sc_dis_ways_mask
      FLAGS_num_sc_dis_ways = mask_dis_ways;
      FLAGS_num_sp_ways     = 0;
      if (FLAGS_rand_sp_ways)
        FLAGS_num_sp_ways = get_rand_sp_ways(nways - FLAGS_num_sc_dis_ways);
      break;

    case 3: // +num_sp_ways, +sc_dis_ways_mask
      if (((mask_dis_ways + sp_ways) < 0) || ((mask_dis_ways + sp_ways) > 24))
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: count(+sc_dis_ways_mask {}) + +num_sp_ways {}, should be between [0,{}]\n", mask_dis_ways, sp_ways, nways);
      FLAGS_num_sc_dis_ways = mask_dis_ways;
      break;

    case 4: // +num_sc_dis_ways
      FLAGS_sc_dis_ways_mask = get_rand_ways_mask(FLAGS_num_sc_dis_ways, nways);
      FLAGS_num_sp_ways = 0;
      if (FLAGS_rand_sp_ways)
        FLAGS_num_sp_ways = get_rand_sp_ways(nways - FLAGS_num_sc_dis_ways);
      break;

    case 5: // +num_sc_dis_ways, +num_sp_ways
      if (((dis_ways + sp_ways) < 0) || ((dis_ways + sp_ways) > 24))
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: +num_sc_dis_ways {} + +num_sp_ways {}, should be between [0,{}]\n", dis_ways, sp_ways, nways);
      FLAGS_sc_dis_ways_mask = get_rand_ways_mask(dis_ways, nways);
      break;

    case 6: // +num_sc_dis_ways, +sc_dis_ways_mask
      if (dis_ways != mask_dis_ways)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: +num_sc_dis_ways {} != count(+num_dis_ways_mask {})\n", dis_ways, mask_dis_ways);
      FLAGS_num_sp_ways = 0;
      if (FLAGS_rand_sp_ways)
        FLAGS_num_sp_ways = get_rand_sp_ways(nways - FLAGS_num_sc_dis_ways);
      break;

    case 7: // +num_sc_dis_ways, +sc_dis_ways_mask, +num_sp_ways
      if (dis_ways != mask_dis_ways)
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: +num_sc_dis_ways {} != count(+num_dis_ways_mask {})\n", dis_ways, mask_dis_ways);
      if (((dis_ways + sp_ways) < 0) || ((dis_ways + sp_ways) > 24))
        cvm::log(cvm::ERROR, "Error: Incompatible plusargs: +num_sc_dis_ways {} + +num_sp_ways {}, should be between [0,{}]\n", dis_ways, sp_ways, nways);
      break;
  }
  cvm::log(cvm::NONE, "[plusargs] +num_sc_dis_ways {} +sc_dis_ways_mask {:#x} +num_sp_ways {}\n",
    FLAGS_num_sc_dis_ways, FLAGS_sc_dis_ways_mask, FLAGS_num_sp_ways);
}

uint32_t
sysmod::get_rand_mask(uint32_t n, uint32_t max)
{
  // Ex: input: n=4, max=8
  // Ex: output: mask=0x9a - bits: 1,3,4,7

  // Create and fill a vector with all possible positions using std::iota
  std::vector<uint32_t> all_positions(max);
  std::iota(all_positions.begin(), all_positions.end(), 0);

  // Shuffle the vector
  std::shuffle(std::begin(all_positions), std::end(all_positions), cvm::rand::gen);

  // Create the mask using the first n elements
  uint32_t mask = 0;
  for (size_t i = 0; i < n; ++i)
    mask |= (1u << all_positions[i]);

  return mask;
}

std::string
sysmod::get_rand_id(uint32_t mask, uint32_t ncores)
{

  // Ex: input: mask=0x9a - available cores: 1,3,4,7
  // Ex: output: hart_enable_id ex: 4,7,1,3 - can be in any order

  // Create and fill a vector with positions from the mask
  std::vector<uint32_t> positions{};
  for (uint32_t i = 0; i < ncores; ++i)
    if ((mask >> i) & 1u)
      positions.push_back(i);

  // Shuffle the vector
  std::shuffle(std::begin(positions), std::end(positions), cvm::rand::gen);

  // String the positions into a list
  std::string result;
  for (size_t i = 0; i < positions.size(); ++i) {
    result += std::to_string(positions[i]);
    if (i < positions.size() - 1) 
      result += ",";
  }
  return result;
}

std::string
sysmod::get_id(uint32_t mask, uint32_t ncores)
{
  // Ex: input: mask=0x9a - available cores: 1,3,4,7
  // Ex: output: hart_enable_id ex: 4,7,1,3 - can be in any order

  // Create and fill a vector with positions from the mask
  std::vector<uint32_t> positions{};
  for (uint32_t i = 0; i < ncores; ++i)
    if ((mask >> i) & 1u)
      positions.push_back(i);

  // String the positions into a list
  std::string result;
  for (size_t i = 0; i < positions.size(); ++i) {
    result += std::to_string(positions[i]);
    if (i < positions.size() - 1) 
      result += ",";
  }
  return result;
}

int32_t
sysmod::get_rand_ways_mask(int32_t n, int32_t max)
{
  int32_t mask = 0;
  uint32_t m = get_rand_mask((uint32_t)n/4, (uint32_t)max/4);
  for (int i=0; i<max/4; ++i)
    if ((m >> i) & 1u)
      mask |= (0xf << (4*i));
  return mask;
}

int32_t
sysmod::get_rand_dis_ways(int32_t nways)
{
  if (nways == 0)
    return nways;

  // Discrete distribution for dis_ways
  // In SC, ways are disabled only in multiples of 4
  // Since we are enabling SC harvest randomization only in select sims,
  // let randomization always disable at least min = 4 ways
  // Keep halving the weights
  // Ex: nways = 24, max = 6, weights = {0.5, 0.25, 0.125 ...}
  // Ex: generated value range gen = [0, 5]
  // Ex: return value = (gen + 1) * 4;
  int32_t max = nways / 4;
  std::vector<double> weights(max);
  for (int i = 0; i < max; ++i) {
      weights[i] = std::pow(0.5, i + 1);
  }

  cvm::rand::discrete_dist<int32_t> dist(weights);
  cvm::log(cvm::MEDIUM, "[random] Probabilities for selecting disabled SC way groups [1..{}] = [{:.2f}]\n",
    max, fmt::join(dist.probabilities(), ", "));
  return (dist() + 1) * 4;
}

int32_t
sysmod::get_rand_sp_ways(int32_t max)
{
  if (max == 0)
    return 0;

  // Discrete distribution for sp_ways
  // In SC, scratch pad ways can be anywhere from 0 to max
  // Let probabilities be biased towards a smaller scratch pad 
  // with mostly up to 4 ways
  // Ex: max = 24, weights = {1.0} upto 4 ways and {0.1} after that
  std::vector<double> weights(max);
  for (int i = 0; i < max; ++i) {
    if (i < 4)
      weights[i] = 1.0;
    else
      weights[i] = 0.1;
  }

  cvm::rand::discrete_dist<int32_t> dist(weights);
  cvm::log(cvm::HIGH, "[random] Probabilities for selecting SP ways [0..{}] = [{:.2f}]\n",
    max, fmt::join(dist.probabilities(), ", "));
  return (dist() + 1);
}

sysmod::~sysmod()
{
}

// forwarding functions for devices
void
sysmod::timer_interrupt(clint::timer_t t) {
      cvm::registry::callbacks.push(
        scope(),
        [t]() {
          cvm::log(cvm::FULL, "[SYSMOD] timer_interrupt [hart={}, mti={} mtime={:#x}]\n", t.hart, t.flag, t.mtime);
          sysmod_timer_interrupt(t.hart, t.flag, t.mtime);
        });
}

void
sysmod::sw_interrupt(clint::sw_t s) {
  cvm::registry::callbacks.push(
      scope(),
      [s]() {
        cvm::log(cvm::FULL, "[SYSMOD] sw_interrupt [hart={}, msi={}]\n", s.hart, s.flag);
        sysmod_sw_interrupt(s.hart, s.flag);
      });
}

void
sysmod::tbox_interrupt(interrupter::interrupt_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] tbox_interrupt [hart={}, intr.sel={:#x}, intr.val={:#x}]\n", i.hart, i.intr_select, i.intr_value);
        sysmod_tbox_interrupt(i.hart, i.intr_select, i.intr_value);
      });
}

void
sysmod::trace_info_handler(trace_cfg::trace_info_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::HIGH, "[SYSMOD] trace_info \n");
        sysmod_trace_info(i.trace_quiesced);
      });
}

void
sysmod::trace_cfg_read_req_router(trace_cfg::trace_cfg_read_t r) {

    transactor::read_t rd;
    rd.addr = r.addr;
    rd.length = r.length;
    rd.id =  r.id;

    auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

    if (this->dev(r.addr)){
        cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
    }

}

//void
// sysmod::scratchpad_xtor_read_req_router(scratchpad_xtor::scratchpad_xtor_read_t r) {

//     transactor::read_t rd; 
//     rd.addr = r.addr;
//     rd.length = r.length;
//     rd.id =  r.id;
//     cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER - addr={:#x} \n", rd.addr);

//     auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

//     if (this->dev(r.addr)){
//     cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER  send to device - addr={:#x} \n", rd.addr);
//         cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
//     }

// }
void
sysmod::uc_helper_backdoor_write(uc_helper::uc_helper_write_t w) {

    if (!FLAGS_bypass_cache && !FLAGS_bypass_mem)
      cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_write: caching is enabled in rv_tester and it does not receive DMAs, so the test could fail if the CPU does a read to this address as it will receive the stale cached data");

    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write addr {:#x} \n",w.addr);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write len {} \n",(unsigned)w.length);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write data-vec : \n");

    for (auto i: w.data)
      cvm::log(cvm::HIGH," {:#x} ",(unsigned)i);

    cvm::log(cvm::FULL, "[SYSMOD] uc_helper_backdoor:write sysmem for addr {:#x}  \n", w.addr);
    transactor::write_t wt;
    wt.addr = w.addr;
    wt.length = 1;
    wt.data = w.data;
    wt.strb = w.strb;

    cvm::log(cvm::FULL, "[UC_HELPER] new backdoor write request at {:#x}", wt.addr);
    if (this->dev(wt.addr))
      cvm::registry::messenger.signal<device::write_t>(this->loc_, {wt});
}

void
sysmod::store_inval_crsp(const inval_crsp_s& payld, bool mcm) {

  cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR::MCM={} FLAGS_MCM={}\n", mcm, FLAGS_mcm );
  if ( mcm and !FLAGS_mcm) return;
  if (!mcm and  FLAGS_mcm) return;
  inval_crsp_ = payld;
  // Compulsive Backdoor write
  uint64_t read_data = 0;
  device::data_t data(8);
  uint64_t ld_addr = ((inval_crsp_.address) >> 6) << 6; // Starting from cacheline base address
  // Performing Whisper Poke for entire Cacheline Granularity
  for (int offset=0; offset<8; offset++) {
    read_data = 0;
    dev("memory")->backdoor_read(ld_addr + (offset*8), 8, data);
    for (int i=0; i<8; ++i) 
      read_data |= uint64_t(data[i]) << (i*8);
     bool valid = true;
     cvm::log(cvm::FULL, "[CBO_INVAL_MONITOR - CRSP POKE] Whisper Poke to Address : {:#x}, with data : {:#x}\n",(ld_addr + (offset*8)),read_data);
     if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (ld_addr + (offset*8)), 8, read_data, valid) || !valid) && FLAGS_whisper_client_check) {
       cvm::log(cvm::ERROR, "Error: store_inval_crsp failed to poke whisper memory");
    }
  }
}

void
sysmod::store_inval_load(const inval_load_s& payload) {
  inval_load_ = payload;
  // Do a backdoor read for the load's address
  device::data_t data(8);
  uint64_t read_data = 0;
  uint64_t ld_addr = inval_load_.address; 
  size_t length;
  length = inval_load_.size;
  int size = length;
  dev("memory")->backdoor_read(ld_addr, length, data);
  for (int i=0; i<size; ++i)
    read_data |= uint64_t(data[i]) << (i*8);

  if(inval_load_.amo) {
    // For AMO MB Bypass -> We dont need to check with main memory contents
    bool valid = true;
    cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR :: Whisper Poke with data:{:#x} for AMO MB Bypass to address:{:#x}\n", inval_load_.data, ld_addr);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', ld_addr, size, inval_load_.data, valid)) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: store_inval_load failed to poke whisper memory in AMO MB Bypass case\n"); 
    }
  }

  if(inval_load_.data == read_data && (!inval_load_.amo)) // Check with main memory for all cases other than AMO mbbypass
  {
    // No need to poke entire cacheline granularity - that will be done after CRSP
    bool valid = true;
    cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR :: Whisper Poke with data:{:#x} for address:{:#x}\n", read_data,ld_addr);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', ld_addr, size, read_data, valid) || !valid) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: store_inval_load failed to poke whisper memory\n"); 
    }
  }
}

cvm::messenger::task<uint64_t>
sysmod::backdoor_write(sysmod::backdoor_write_t t) {

  cvm::log(cvm::HIGH, "[BACKDOOR_WRITE] new backdoor write request at {:#x} value:{:#x} size: {:#x}\n", t.address, t.data, t.size);
  device::data_t datax(8);
  device::strb_t strbx(8);

  if (FLAGS_cosim) {
    bool valid = true;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', t.address, 8, t.data, valid) || !valid) && FLAGS_whisper_client_check)
      cvm::log(cvm::ERROR, "Error: backdoor_write failed to poke whisper memory\n");
  }
    
  for (int i=0; i<t.size; ++i, t.data>>=8) {
    datax[i] = t.data & 0xff;
    strbx[i] = true;
  }
  dev("memory")->backdoor_write(t.address, t.size, datax, strbx);
  co_return 0;
}

cvm::messenger::task<uint64_t>
sysmod::backdoor_read(uint64_t address) {

    device::data_t data(8);
    dev("memory")->backdoor_read(address, 8, data);
    uint64_t read_data = 0;
    for (int i=0; i<8; ++i)
      read_data |= uint64_t(data[i]) << (i*8);
    co_return read_data;
}

void
sysmod::uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t r) {
    if (!FLAGS_bypass_cache && !FLAGS_bypass_mem)
      cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_read: caching is enabled in rv_tester and it does not receive DMAs, so this backdoor read might receive stale data");

    cvm::log(cvm::HIGH, "[SYSMOD] uc_helper_backdoor_read addr {:#x} \n", r.addr);
    cvm::log(cvm::HIGH, "[SYSMOD] uc_helper_backdoor_read len {} \n", (unsigned)r.length);
    cvm::log(cvm::HIGH, "New PRT BACKDOOR read request at {:#x}", r.addr);
    device::data_t data(8);
    std::vector<uint8_t> data_trickbox(8);
    std::vector<bool> strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
      // Read from memory and write to requested dev tag
      if (not dev("memory"))
        return;
      dev("memory")->backdoor_read(r.addr, r.length, data);
      for (size_t i = 0; i < 8; i++)
        data_trickbox[i] = (uint8_t)data[i];

      auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
      cvm::registry::messenger.signal(tbox_loc, uc_helper::trickbox_mem_req_t{r.addr, r.length, data_trickbox, strb});
}

void
sysmod::dmi_write(debugger::dmi_data_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] trickbox::dmi.(upper,lower) = {:#x}, {:#x}\n", i.upper_dmi_data, i.lower_dmi_data);
        sysmod_dmi_write(i.hart,i.upper_dmi_data,i.lower_dmi_data);
      });
}

void
sysmod::terminate(htif::terminate_t t) {
  // fast path for handlers which want to be notified immediately
  cvm::registry::messenger.signal<rv_tester::terminate_called_fast>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called_fast{});
  // we want this to be low prio and async so it goes behind existing rvfi transactions in the queue
  // because of QoS this could have been seen before all rvfi transactions up to this instruction were processed
  // unless the terminator tells us that it came from a low priority transaction
  const auto prio = t.low_priority_based ? cvm::messenger::highest_priority : cvm::messenger::lowest_priority;
  if (t.low_priority_based)
    cvm::registry::messenger.signal<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{});
  else
    cvm::registry::messenger.signal_async<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{}, prio);

  if (FLAGS_sysmod_terminate) {
      cvm::registry::callbacks.push(
          scope(),
          sysmod_terminate
      );
  }
}

void
sysmod::reset() {
  compose();
  load_prog(FLAGS_hex, FLAGS_load, FLAGS_load_lz4, FLAGS_load_bin);
  load_io(FLAGS_load_io);
  load_boot(FLAGS_bootrom_path);
  load_csr_mmr_boot(1);
  load_cplfw(FLAGS_cplfw_path);
  set_secure_region(FLAGS_stee_secure_region);
}
void
sysmod::set_secure_region(std::string region) {
  if (region == "")
    return;
  stee = true;
  std::vector<std::string> secure_region = cosim_util::split_string(region, ":");
  secure_region_start_ = std::stoull(secure_region.at(0), nullptr, 0);
  secure_region_end_   = std::stoull(secure_region.at(1), nullptr, 0);
  cvm::registry::messenger.call<whisperClient<uint64_t>::secureRegionRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), secure_region_start_, secure_region_end_);
}

std::shared_ptr<TT_APLIC::Aplic>
sysmod::create_aplic() const {
  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  std::vector<TT_APLIC::DomainParams> domains;

  // Aplic is handled specially because we must look through all the aplic
  // domains before constructing it.
  for (const auto &d : memmap_) {
    const auto type = d.second.type;

    if (type != "aplic_domain")
      continue;

    const auto base = d.second.base;
    const auto size = d.second.size;
    const auto tag  = d.second.tag;
    const auto attributes = d.second.attributes;

    if (attributes.is_null()) {
      throw std::runtime_error("Domain " + tag + " does not have attributes");
    }

    const auto parent_json = attributes.at("parent");
    const std::optional<std::string> parent = parent_json.is_null() ?
      static_cast<std::optional<std::string>>(std::nullopt) :
      parent_json.template get<std::string>();
    const size_t child_index = attributes.at("child_index");
    const auto privilege = attributes.at("is_machine") ?
      TT_APLIC::Privilege::Machine :
      TT_APLIC::Privilege::Supervisor;

    domains.emplace_back(tag, parent, child_index, base, size, privilege);
  }

  if (domains.empty()) {
    return nullptr;
  }

  std::shared_ptr<TT_APLIC::Aplic> aplic =
    std::make_shared<TT_APLIC::Aplic>(nharts, FLAGS_aplic_sources, domains);

  auto axi_mst_loc = masters[0];
  auto msiCallback = [axi_mst_loc] (uint64_t addr, uint32_t data) {
    cvm::log(cvm::DEBUG, "Aplic sent IMSIC interrupt {:#x} (@ {:#x})\n", data, addr);

    std::vector<uint8_t> data_vec(64, 0);
    std::vector<bool> strb(64, false);
    for (int i = 0; i < 4; i++) {
      data_vec[i] = data & 0xff;
      strb[i] = true;
      data >>= 8;
    }

    cvm::registry::messenger.signal(axi_mst_loc,
        transactor::write_request_t{addr, 64, data_vec, strb, true });
    return true;
  };
  aplic->setMsiCallback(msiCallback);

  return aplic;
}


void
sysmod::compose() {

  devices_.clear();
  // Load memmap
  if (!cvm::registry::messenger.call<memmap::getRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.MEMMAP", 0), memmap_))
    return;

  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  std::shared_ptr<mem_manager> mm = std::make_shared<mem_manager>(mem_manager::opts{.page_size = FLAGS_mem_manager_page_size});

  try {
    std::shared_ptr<TT_APLIC::Aplic> aplic = create_aplic();

    for (const auto& d : memmap_) {
      const auto base = d.second.base;
      const auto size = d.second.size;
      const auto type = d.second.type;
      const auto tag  = d.second.tag;

      std::unique_ptr<device> device;

      if (type == "memory") {
        device = std::make_unique<sysmod_mem>(tag, base, size, loc_, mm);

      } else if (type == "io_dev") {
        device = std::make_unique<io_dev>(tag, base, size, loc_);

      } else if (type == "null_dev") {
        device = std::make_unique<null_dev>(tag, base, size, loc_);

      } else if (type == "htif") {
        device = std::make_unique<htif>(tag, base, loc_);
        cvm::registry::messenger.connect<htif::terminate_t>(
            loc_,
            [&](htif::terminate_t t) { return this->terminate(t); });

      } else if (type == "uart8250") {
        device = std::make_unique<io_device<WdRiscv::Uart8250>>(tag, loc_,
            base, 32, aplic, FLAGS_uart8250_iid,
            std::make_unique<WdRiscv::PTYChannel>());
      } else if (type == "dm") {
        // TODO: cvm::ERROR
       // assert(masters.size() > 0);
       // device = std::make_unique<dm>(tag, base, size, loc_, masters[0]);
      } else if (type == "trace_cfg") {
        // TODO: cvm::ERROR
        cvm::registry::messenger.connect<trace_cfg::trace_info_t>(
            loc_,
            [&](trace_cfg::trace_info_t i) { return this->trace_info_handler(i); });
        assert(masters.size() > 0);
        device = std::make_unique<trace_cfg>(tag, base, size, loc_, masters[0]);
        // TODO: cvm::ERROR

      // } else if (type == "scratchpad_xtor") {
      //   // TODO: cvm::ERROR
      //   assert(masters.size() > 0);
      //   device = std::make_unique<scratchpad_xtor>(tag, base, size, loc_, masters[0]);
      //   cvm::registry::messenger.connect<scratchpad_xtor::scratchpad_xtor_read_t>(
      //       loc_,
      //       [&](scratchpad_xtor::scratchpad_xtor_read_t i) { return this->scratchpad_xtor_read_req_router(i); });

      } else if (type == "clint") {
        device = std::make_unique<clint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });

      } else if (type == "aclint") {
        device = std::make_unique<aclint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });

      } else if (type == "trickbox") {
        device = std::make_unique<trickbox>(tag, base, nharts, loc_,masters[0]);
        cvm::registry::messenger.connect<interrupter::interrupt_t>(
            loc_,
            [&](interrupter::interrupt_t i) { return this->tbox_interrupt(i); });
        cvm::registry::messenger.connect<debugger::dmi_data_t>(
            loc_,
            [&](debugger::dmi_data_t i) { return this->dmi_write(i); });
       // cvm::registry::messenger.connect<jtag_driver::jtag_data_t>(
       //     loc_,
       //     [&](jtag_driver::jtag_data_t i) { return this->jtag_req(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_write_t>(
            loc_,
            [&](uc_helper::uc_helper_write_t i) { return this->uc_helper_backdoor_write(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_read_req_t>(
            loc_,
            [&](uc_helper::uc_helper_read_req_t i) { return this->uc_helper_backdoor_read(i); });
        cvm::registry::messenger.connect<trace_cfg::trace_cfg_read_t>(
            loc_,
            [&](trace_cfg::trace_cfg_read_t i) { return this->trace_cfg_read_req_router(i); });

      } else if (type == "aplic_domain") {
        device = std::make_unique<aplic_device>(tag, base, size, loc_, aplic);
      } else {
        cvm::log(cvm::ERROR, "Error: unknown sysmod type {} \n", type);
      }

      devices_.emplace_back(std::move(device));
    }

    devices_.emplace_back(std::make_unique<heartbeat>("heartbeat", 0, 0, loc_));

    assert(masters.size() > 0);
  }
  catch (std::exception& e) {
    std::cerr << "Error: Memmap access exception.\n" << "  Message: " << e.what() << "\n";
  }
}

device*
sysmod::dev(uint64_t addr) {
  if (stee) {
    if (addr & FLAGS_pa_mask)
      addr = addr & ~FLAGS_pa_mask;
  }
  for (auto& d : devices_) {
    if (d->has_addr(addr))
      return d.get();
  }
  cvm::log(cvm::ERROR, "Error: Address not mapped: {:#x}\n", addr);
  return nullptr;
}

device*
sysmod::dev(const std::string& tag) {

  for (auto& d : devices_) {
    if (d->tag() == tag)
      return d.get();
  }
  cvm::log(cvm::ERROR, "Error: Tag not mapped: {}\n", tag);
  return nullptr;
}

void
sysmod::load_io(const std::string& io) {

  if (io == "")
    return;

  std::stringstream ss(io);
  while (ss.good()) {
    std::string io_str;
    std::getline(ss, io_str, ',' );
    std::size_t pos = io_str.find(':');
    if (pos != std::string::npos) {
      std::string tag = io_str.substr(0, pos);
      std::string offset_str = io_str.substr(pos + 1);
      uint64_t offset = std::stoul(offset_str, nullptr, 16);

      device::data_t data(8);
      device::strb_t strb(8);
      for (size_t i = 0; i < 8; i++) strb[i] = true;

      // Read from memory and write to requested dev tag
      if (not dev("memory") or not dev(tag))
        return;

      dev("memory")->backdoor_read(dev(tag)->addr()+offset, 8, data);
      dev(tag)->backdoor_write(dev(tag)->addr()+offset, 8, data, strb);
    }
  }
}

bool
sysmod::bin_load(const std::string load, bool lz4_compressed)
{
  uint64_t offset = 0;
  std::string file = load;

  cvm::log(cvm::MEDIUM, "Loading {}\n", load);
  if (std::size_t pos = load.find(':'); pos != std::string::npos) {
    file = load.substr(0, pos);
    std::string offset_str = load.substr(pos + 1);
    offset = std::stoull(offset_str, nullptr, 0);
  }
  if (not dev("memory") or not
          (lz4_compressed ? dynamic_cast<sysmod_mem&>(*dev("memory")).init_lz4(file, offset) :
                            dynamic_cast<sysmod_mem&>(*dev("memory")).init_bin(file, offset)
          )
     ) {
    cvm::log(cvm::ERROR, "No memory defined");
    return false;
  }

  return true;
}

void
sysmod::load_prog(const std::string& hex, const std::string& load, const std::string& lz4, const std::string& bin) {

  for (const auto& d : memmap_) {
    const auto type = d.second.type;
    const auto tag  = d.second.tag;

    if (type != "memory") continue;

    if (load != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", load);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_elf(load)) {
        cvm::log(cvm::ERROR, "Failed to load program");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", load);
    }

    if (hex != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", hex);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_hex(hex)) {
        cvm::log(cvm::ERROR, "No memory defined");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", hex);
    }

    auto parse_bin = [this](const std::string& flag, bool lz4_compressed) {
      std::stringstream ss;

      cvm::log(cvm::MEDIUM, "Parsing {}\n", flag);
      // split string by colon into file path and offset
      // if no colon is found, assume offset is 0

      ss << flag;
      while (ss.good())
      {
        std::string substr;

        getline(ss, substr, ',');
        if (not bin_load(substr, lz4_compressed))
          return;
      }
      cvm::log(cvm::MEDIUM, "Parsing {} complete\n", flag);
    };

    if (lz4 != "") parse_bin(lz4, true);
    if (bin != "") parse_bin(bin, false);

    // all memories share the same backing mem manaager
    return;
  }

  cvm::log(cvm::ERROR, "No memory found\n");
}

void
sysmod::load_boot(const std::string& boot) {

  if (FLAGS_bootrom && boot != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", boot);
    if (boot.substr(boot.length() - 3) == "elf") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_elf(boot)) {
        cvm::log(cvm::ERROR, "No boot defined");
        return;
      }
    }
    if (boot.substr(boot.length() - 3) == "hex") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_hex(boot)) {
        cvm::log(cvm::ERROR, "No boot defined");
        return;
      }
    }

    // Write num_harts for bootrom to access
    // Write hart_sync_en for bootrom to access
    // Write SP init
    // Write SP ways
    // Write STEE swid
    device::data_t data(8);
    device::strb_t strb(8);
    for (size_t i = 0; i < 8; i++) data[i] = 0;
    for (size_t i = 0; i < 8; i++) strb[i] = true;

    data [0] = FLAGS_num_harts;
    dev("boot")->backdoor_write(dev("boot")->addr() + boot_num_harts_offset, 8, data, strb);

    data[0] = uint8_t(FLAGS_hart_sync_en);
    dev("boot")->backdoor_write(dev("boot")->addr() + boot_hart_sync_en_offset, 8, data, strb);

    if (FLAGS_enable_sp_init) {
      data[0] = 0x1;
      dev("boot")->backdoor_write(dev("boot")->addr() + boot_sp_init_offset , 8, data, strb);
    }
    if (FLAGS_num_sp_ways > 24) {
      cvm::log(cvm::ERROR, "Error: Maximum 24 sharedcache ways can be alloted as Scratchpad \n");
      return;
    }
    data[0] = uint8_t(FLAGS_num_sp_ways);
    dev("boot")->backdoor_write(dev("boot")->addr() + boot_sp_ways_offset, 8, data, strb);

    data[0] = FLAGS_matp_swid;
    dev("boot")->backdoor_write(dev("boot")->addr() + boot_matp_swid_offset, 8, data, strb);
  }
}

void
sysmod::load_csr_mmr_boot(uint64_t dut) {
  if (!FLAGS_bootrom || (FLAGS_set_csr == "" && FLAGS_set_mmr == ""))
      return;

  int addr;
  auto add_to_mem = [&addr, &dut, this] (const uint32_t op) {
    if (dut) {
      device::strb_t strb(4);
      for (size_t i = 0; i<4; i++) strb[i] = true;
      cvm::log(cvm::HIGH, "Address: {:#x} OPCODE: {:#x}\n",addr, op);
      device::data_t data(4);
      for (size_t i=0; i<4; i++) data[i] = op >> 8*i;
      dev("boot")->backdoor_write(addr, 4, data, strb);

    } else {
      bool valid = true;
      if (FLAGS_cosim && (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', addr, op, valid) || !valid) && FLAGS_whisper_client_check)
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
    }
    addr += 4;
  };

  if (FLAGS_set_mmr != "") {
    cvm::log(cvm::HIGH, "Backdoor writes to MMR addresses\n");
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> mmr_data;
    std::map<std::string, uint64_t> mmr_map;
    for (const auto& mmr : mmrs)
      mmr_map[mmr.name] = mmr.address;
    try {
      std::vector<std::string> mmr_vals = cosim_util::split_string(FLAGS_set_mmr, ",");
      for (const auto& entry : mmr_vals) {
        std::vector<std::string> mmr_val = cosim_util::split_string(entry, ":");

        auto mmr = mmr_val.at(0);
        if (mmr_map.count(mmr)) {
          addr = mmr_map[mmr];

        } else if (mmr.find("slice") != std::string::npos) {
          std::vector<std::string> mmr_vec = cosim_util::split_string(entry, "_slice");
          addr = mmr_map[mmr_vec.at(0)];
          auto slice = std::stoull(mmr_vec.at(1), nullptr, 0);
          addr = addr + slice * 0x1000;

        } else {
          addr = std::stoull(mmr, nullptr, 0);
        }

        auto size  = std::stoull(mmr_val.at(1), nullptr, 0);
        auto value = std::stoull(mmr_val.at(2), nullptr, 0);
        if (!size || size>8 || (size & (size-1))) {
          cvm::log(cvm::ERROR, "Error: MMR size should be 1,2,4,8. see string:{}\n", entry);
          return;
        }
        mmr_data.push_back(std::make_tuple(addr, size, value));
      }
    }
    catch (...) {
      cvm::log(cvm::ERROR, "Error: unable to parse +set_mmr={}\n", FLAGS_set_mmr);
      return;
    }
    int dest_gpr_addr = 28, dest_gpr_value = 29, temp_gpr2 = 30, temp_gpr3 = 31;
    int store_opcode = 0x23;
    addr = dev("boot")->addr() + boot_rand_mmr_offset;
    for (auto &mmr_val : mmr_data) {
      auto addr  = std::get<0>(mmr_val);
      auto size  = std::get<1>(mmr_val);
      auto value = std::get<2>(mmr_val);
      std::vector<uint32_t> opcodes  = cosim_util::opcode_move_value_to_register(addr, dest_gpr_addr, temp_gpr2, temp_gpr3);
      for (auto& opcode: opcodes) add_to_mem(opcode);
      opcodes = cosim_util::opcode_move_value_to_register(value, dest_gpr_value, temp_gpr2, temp_gpr3);
      for (auto& opcode: opcodes) add_to_mem(opcode);
      uint32_t store_op = store_opcode + (dest_gpr_value<<20) + (dest_gpr_addr<<15);
      if (size == 8)
       store_op |= (0b011)<<12;
      else if (size == 4)
        store_op |= (0b010)<<12;
      else if (size == 2)
        store_op |= (0b001)<<12;
      add_to_mem(store_op);
    }
  add_to_mem(opcode_ret);
  }

  if (FLAGS_set_csr != "") {
    std::map<uint64_t,    uint64_t> csr_data;
    std::map<std::string, uint64_t> csr_map;
    for (const auto& csr : csrs)
      csr_map[csr.second.name] = csr.second.addr;

    try { // parse the +set_csr and report any errors
      std::string delimiter = ",";
      std::vector<std::string> csr_num_val = cosim_util::split_string(FLAGS_set_csr, delimiter);
      for (const auto& entry : csr_num_val) {
        delimiter = ":";
        std::vector<std::string> num_val = cosim_util::split_string(entry, delimiter);
        auto csr = num_val.at(0); // expect both csr address("0x301") as well as string("misa")
        auto value = std::stoull(num_val.at(1), nullptr, 0);
        if (csr_map.count(csr))
          csr_data[csr_map[csr]] = value;
        else {
          char* p;
          uint64_t csrn = std::strtoul(csr.c_str(), &p, 0);
          if (*p == 0)
            csr_data[csrn] = value;
          else {
            cvm::log(cvm::ERROR, "Error: csr_name:{} undefined see +set_csr switch\n", csr);
            return;
          }
        }
      }
    }
    catch (...) {
      cvm::log(cvm::ERROR, "Error: unable to parse +set_csr={}\n", FLAGS_set_csr);
      return;
    }

    cvm::log(cvm::HIGH, "Backdoor bootrom CSR writes to memory\n");
    int csr_opcode = 0x73;
    addr = dev("boot")->addr() + boot_rand_csr_offset;
    for (auto const& [csr_num, value] : csr_data) {
      uint32_t csr_op = 0;
      if (value >= 32) {
        int dest_gpr = 4, temp_gpr2 = 3, temp_gpr3 = 28;
        std::vector<uint32_t> opcodes = cosim_util::opcode_move_value_to_register(value, dest_gpr, temp_gpr2, temp_gpr3);
        for (auto& opcode: opcodes) add_to_mem(opcode);
        csr_op = csr_opcode + (0/*x0*/<<7) + (1<<12) + (dest_gpr<<15) + (csr_num<<20);
      } else
        csr_op = csr_opcode + (0/*x0*/<<7) + (5<<12) + (value<<15) + (csr_num<<20);
      add_to_mem(csr_op);
    }
  add_to_mem(opcode_ret);
  }
}

void
sysmod::load_cplfw(const std::string& cplfw) {

  if (FLAGS_cplfw && cplfw != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", cplfw);
    if (cplfw.substr(cplfw.length() - 3) == "elf") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_elf(cplfw)) {
        cvm::log(cvm::ERROR, "No cpl firmware defined");
        return;
      }
    }
    if (cplfw.substr(cplfw.length() - 3) == "hex") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_hex(cplfw)) {
        cvm::log(cvm::ERROR, "No cpl firmware defined");
        return;
      }
    }
  }
}

void
sysmod::store_dm_rand() {

  std::string whisper_client = "TOP.PLATFORM.WHISPER_CLIENT";
  uint64_t addr = cvm::registry::messenger.call<whisperClient<uint64_t>::get_dm_rand_addr_RPC>(cvm::topology::get_from_hierarchy(whisper_client, 0));
  auto dm_rand_values = cvm::registry::messenger.call<whisperClient<uint64_t>::get_dm_rand_val_RPC>(cvm::topology::get_from_hierarchy(whisper_client, 0));
  device::data_t dataw(8);
  device::strb_t strb(8);
  for (const auto &val:dm_rand_values) {
    for (size_t i=0; i<8; i++) {
      dataw[i] = (val >> 8*i) & 0xff;
      strb[i]  = true;
    }
    dev("trickbox")->backdoor_write(addr, 8, dataw, strb);
    addr += 8;
  }
}

void
sysmod::tick(uint64_t advance) {

  ticks_ += advance;
  advance = 0;
  if (ticks_ >= FLAGS_sysmod_tick_update_threshold)  {
      auto rem = ticks_ % FLAGS_sysmod_tick_update_threshold;
      advance  = ticks_ - rem;
      ticks_   = rem;
  }
  if (advance)
    for (auto& d : devices_) {
     if (!cosim_init_ && d->tag() == "trickbox")
         continue; // when in cosim mode, do not initialize ticks of trickbox until Whisper client is set.
      d->tick(advance);
    }
}

void
sysmod::is_dut_reset_req(bool dut_reset_req,uint64_t clocks,uint64_t divisor) { 

  cvm::log(cvm::FULL,"Value of dut_reset_req in sysmod is : {}\n",dut_reset_req);
  if (dut_reset_req)
    for (auto& d : devices_)
      d->is_dut_reset_req(dut_reset_req,clocks,divisor);
}

void
sysmod::jtag_tick(uint64_t advance) {

  jtag_ticks_ += advance;
  if (advance)
    for (auto& d : devices_)
      d->jtag_tick(advance);
}

void sysmod::tboxtrig_updatemem(uint64_t addr, uint64_t data) {

    device::data_t dataw(8);
    device::strb_t strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
    for (size_t i = 0; i < 8; i++) dataw[i] = (data >> 8*i) & 0xff;

    dev("trickbox")->backdoor_write(addr, 8, dataw, strb);
}

void sysmod::overlay_tick(uint64_t advance) {

  overlay_ticks_ += advance;
   if (advance)
     for (auto& d : devices_)
       d->overlay_tick(advance);
}


extern "C" {
  void sysmod_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

  uint64_t backdoor_read(uint64_t address) {
    uint64_t out_data;
    std::atomic<bool> flag{false};
    auto loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal_async<sysmod::backdoor_read_t>(loc_, sysmod::backdoor_read_t{address, &flag, &out_data});
    flag.wait(false);
    return out_data;
  }
  void backdoor_write(uint64_t address, uint64_t data) {
    std::atomic<bool> flag{false};
    auto loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal_async<sysmod::backdoor_write_t>(loc_, sysmod::backdoor_write_t{address, data, 8, &flag});
    flag.wait(false);
    return;
  }
}
