#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "src/sysmod/trickbox/post_si_pcietc_helper.h"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"

struct post_si_pcietc_helper_rpc_data_t {
  uint64_t addr;
  uint64_t offset;
  size_t length;
  data_t data;
};
// CVM procedure call declaration
CVM_MESSENGER_procedure_call(post_si_pcietc_helper_write_rpc, void(const post_si_pcietc_helper_rpc_data_t&));
CVM_MESSENGER_procedure_call(post_si_pcietc_helper_read_rpc, void(post_si_pcietc_helper_rpc_data_t&));

post_si_pcietc_helper::post_si_pcietc_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
    : subdevice(tag, addr, 0xc00000, loc) {
  rng.seed(FLAGS_seed);
  post_si_pcietc_helper_base = addr;

  // Get post_si_pcietc_sequence location
  overlay_driver_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.OVERLAY_DRIVER", 0);

  reset();
  checkUsage();
}

post_si_pcietc_helper::~post_si_pcietc_helper() {
}

void post_si_pcietc_helper::checkUsage() {
  //For Future FLAG usage
}

void post_si_pcietc_helper::read_dev(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr)) {
    return;
  }
  cvm::log(cvm::HIGH, "[post_si_pcietc_helper] read addr: {:#x} length: {}\n", addr, length);
  post_si_pcietc_helper_rpc_data_t signal_data;
  signal_data.addr = addr;
  signal_data.offset = addr - post_si_pcietc_helper_base;
  signal_data.length = length;
  signal_data.data.assign(length, 0);
  cvm::registry::messenger.call<post_si_pcietc_helper_read_rpc>(overlay_driver_loc_, signal_data);

  data = signal_data.data;
}

void post_si_pcietc_helper::write(uint64_t addr, size_t length, const data_t& data, const strb_t&) {
  if (not has_addr(addr))
    return;
  cvm::log(cvm::HIGH, "[post_si_pcietc_helper] write addr: {:#x} length: {}\n", addr, length);
  post_si_pcietc_helper_rpc_data_t signal_data;
  signal_data.addr = addr;
  signal_data.offset = addr - post_si_pcietc_helper_base;
  signal_data.length = length;
  signal_data.data = data;
  cvm::registry::messenger.call<post_si_pcietc_helper_write_rpc>(overlay_driver_loc_, signal_data);
}
