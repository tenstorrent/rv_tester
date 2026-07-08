#include "mcmi.h"
#include "rv_tester_plusargs.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/registry.hpp"
#include "whisper_client.h"
#include "src/transactors/axi_sw/axi.h"
#include "rvfi_plusargs.h"
#include "bridge_plusargs.h"
#include <bitset>
#include <vector>
#include <algorithm>
#include <cassert>

DEFINE_bool(mcm, true, "Enable mcm");

mcmi::mcmi(cvm::topology::loc_t loc, unsigned id, std::shared_ptr<bridge> bridge)
    : log("h" + std::to_string(id) + "_dut_mcmi.log"), loc_(loc), id_(id), bridge_(bridge) {

  cvm::log(cvm::MEDIUM, "[MCMI loc {} id{}] Constructing mcm...\n", loc_, id_);
}

void mcmi::configure() {

  connect<
      rv_tester_transactions::cosim::m_reset<>,
      rv_tester_transactions::cosim::m_trap<>,
      rv_tester_transactions::cosim::m_rvfi<>,
      rv_tester_transactions::cosim::m_mcmi_read<>,
      rv_tester_transactions::cosim::m_mcmi_insert<>,
      rv_tester_transactions::cosim::m_mcmi_write<>,
      rv_tester_transactions::cosim::m_mcmi_bypass<>,
      rv_tester_transactions::cosim::m_mcmi_ifetch_req<>,
      rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>,
      rv_tester_transactions::cosim::m_mcmi_ievict<>,
      rv_tester_transactions::cosim::m_mcmi_devict<>,
      rv_tester_transactions::cosim::m_mcmi_flush<>,
      rv_tester_transactions::cosim::m_mcmi_writeback<>,
      rv_tester_transactions::cosim::m_mcmi_dfetch<>,
      bridge::error_loc>(loc_);

  connect<
      rv_tester::terminate_called,
      rv_tester::terminate_called_mem_checks>(cvm::topology::get_from_type("PLATFORM", 0));
}

mcmi::~mcmi() {
}

void mcmi::check() {
  // Check implementation if needed
}

void mcmi::process(const rv_tester_transactions::cosim::m_reset<>& m_reset) {

  if (loc_ != m_reset.location)
    return;

  in_reset_ = false;
  cvm::log(cvm::MEDIUM, "[mcmi] reset\n");
}

void mcmi::process(const rv_tester_transactions::cosim::m_trap<>& m_trap) {

  if (in_reset_)
    return;

  if (loc_ != m_trap.location)
    return;

  if (m_trap.id == EXCP) {

    ecause_ = m_trap.cause & 0xff;
    if (FLAGS_cosim && ecause_ == 60) {
      cvm::log(cvm::HIGH, "Enter patch via exception\n");
      if (FLAGS_cosim)
        bridge_->set_patch_mode(ENTER_PATCH);
      patch_mode_ = true;
    } else if (ecause_ == CUSTOM_VEC_CMODE) {
      if (!(vec_cmode_ && (m_trap.pc_addr == vec_cmode_pc_addr_))) {
        vec_cmode_ = true;                   // RVTOOLS-3265, RVTOOLS-3479: Adjust tag for conservative mode vector instructions
        vec_cmode_first_tag_ = m_trap.order; // Capture the tag and use it for all activity related to the vector instruction
        vec_cmode_pc_addr_ = m_trap.pc_addr;
      }
      // RVDE-24355: Store memory error for conservative mode vector instruction
      if (mem_error_) {
        vec_cmode_mem_errors_[vec_cmode_first_tag_] = true;
      }
    } else if (vec_cmode_ && (vec_cmode_tags_.find(m_trap.order) == vec_cmode_tags_.end())) {
      vec_cmode_tags_.emplace(m_trap.order, vec_cmode_first_tag_); // Capture the tag of any exceptions that happen in the shadow of conservative mode
    }
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (in_reset_)
    return;

  if (loc_ != m_rvfi.location)
    return;

  if (m_rvfi.set_pmode) { // when we enter patch mode via ucode
    patch_mode_ = true;
    patch_mode_first_tag_ = m_rvfi.order;
  }
  if (m_rvfi.clr_pmode) {
    patch_mode_ = false;
    patch_mode_first_tag_ = 0;
  }
  if (patch_mode_) {
    if (!patch_mode_first_tag_) {
      patch_mode_first_tag_ = m_rvfi.order;
    }
    if (patch_mode_tags_.find(m_rvfi.order) == patch_mode_tags_.end())
      patch_mode_tags_.emplace(m_rvfi.order, patch_mode_first_tag_);
  }

  if (vec_cmode_ && vec_cmode_tags_.find(m_rvfi.order) == vec_cmode_tags_.end())
    vec_cmode_tags_.emplace(m_rvfi.order, vec_cmode_first_tag_);

  if (!m_rvfi.last_uop)
    return;

  vec_cmode_ = false;

  // RVDE-24355: Clean up conservative mode memory errors when vec_cmode_ is cleared
  if (!vec_cmode_ && !vec_cmode_mem_errors_.empty()) {
    cvm::log(cvm::HIGH, "[RVDE-24355] Clearing vector conservative mode memory errors\n");
    vec_cmode_mem_errors_.clear();
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read) {
  if (in_reset_)
    return;

  if (patch_access(m_mcmi_read.addr))
    return;

  mem_t m;
  bool cacheable = (m_mcmi_read.attr & 0x1000) == 0x1000;
  m.valid = true;
  m.hart = m_mcmi_read.hart;
  m.cycle = m_mcmi_read.cycle;
  m.opcode = m_mcmi_read.opcode;
  m.tag = m_mcmi_read.order;
  m.pa = m_mcmi_read.addr;
  m.size = std::popcount(m_mcmi_read.mask);
  m.data = m_mcmi_read.data;
  m.data_vec = m_mcmi_read.data_vec;
  m.amo = m_mcmi_read.amo;
  m.amo_op = m_mcmi_read.amo_op;
  m.v_ext = m_mcmi_read.v_ext;
  m.field = m_mcmi_read.field;
  m.elem_idx = m_mcmi_read.elem_idx;
  m.splat = m_mcmi_read.splat;
  uint8_t elemsize = m_mcmi_read.elem_size;

  // Handle SC
  // If read before bypass, store pass/fail result
  // If bypass before read, check pass/fail result and send/don't send bypass
  if (m.amo && m.amo_op == SC) {
    if (sc_bypass_.find(m.tag) == sc_bypass_.end()) {
      sc_result_.emplace(m.tag, m);
    } else {
      if (!sc_failed(sc_bypass_.at(m.tag))) {
        bridge_->process_dut_mcm_bypass(m.hart, sc_bypass_.at(m.tag), true);
        sc_bypass_.erase(m.tag);
      }
    }
    return;
  }

  std::bitset<256> data_vec = m.data_vec;

  process_memory_access(
      m_mcmi_read.addr,
      m_mcmi_read.mask,
      m_mcmi_read.data_vec,
      [&]() {
        process_read_single_consecutive(m, m_mcmi_read, data_vec, elemsize, cacheable);
      },
      [&](uint64_t start_addr, size_t size, const std::bitset<256>& data_vec, const std::string& hex_string) {
        process_read_split_range(m, m_mcmi_read, start_addr, size, data_vec, hex_string, elemsize, cacheable);
      });
  if (m.amo && m.amo_op != LR && FLAGS_emulate_amo_arithmetic) {
    process_amo(m);
  }
}

// Helper function to convert a hex string into a bitset
std::bitset<256> mcmi::stringToBitset(const std::string& hexString) {
  std::bitset<256> bits;
  size_t len = hexString.length();
  for (size_t i = 0; i < len; ++i) {
    int hexDigit = (hexString[len - 1 - i] >= '0' && hexString[len - 1 - i] <= '9')
                       ? hexString[len - 1 - i] - '0'
                       : hexString[len - 1 - i] - 'a' + 10;
    for (int j = 3; j >= 0; --j) {
      bits[(i * 4) + j] = (hexDigit >> j) & 1;
    }
  }
  return bits;
}

void mcmi::process_read_single_consecutive(mem_t& m, const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read,
                                           const std::bitset<256>& data_vec, uint8_t elemsize, bool cacheable) {
  if (m_mcmi_read.v_ext & m_mcmi_read.splat) {
    uint16_t total_elements;
    uint64_t numones = std::popcount(m_mcmi_read.mask);
    if (numones / elemsize) {
      total_elements = numones / elemsize;
      m.size = elemsize;
    } else {
      total_elements = 1;
      m.size = numones;
    }
    for (int i = 0; i < total_elements; i++) {
      uint64_t value = 0;
      // Extract the bits for the current element
      for (size_t j = 0; j < elemsize * 8; ++j) {
        size_t bit_index = i * elemsize * 8 + j;
        if (bit_index >= data_vec.size())
          break; // Avoid overflow
        if (data_vec[bit_index]) {
          value |= (1ULL << j);
        }
      }
      m.data_vec = value;
      m.elem_idx = m_mcmi_read.elem_idx + i;
      bridge_->process_dut_mcm_read(m_mcmi_read.hart, m, cacheable);
    }
  } else {
    m.data_vec = extract_bits_as_bitset(m.data_vec, m.size * 8, 0);
    bridge_->process_dut_mcm_read(m_mcmi_read.hart, m, cacheable);
  }
}

void mcmi::process_read_split_range(mem_t& m, const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read,
                                    uint64_t start_addr, size_t size, const std::bitset<256>& data_vec,
                                    const std::string& hex_string, uint8_t elemsize, bool cacheable) {
  if (m_mcmi_read.v_ext & m_mcmi_read.splat) {
    uint16_t total_elements;
    if (size / elemsize) {
      total_elements = size / elemsize;
      m.size = elemsize;
    } else {
      total_elements = 1;
      m.size = size;
    }
    m.pa = m_mcmi_read.addr;
    for (int i = 0; i < total_elements; i++) {
      size_t start, end;
      if (size / elemsize) {
        start = hex_string.size() - (i + 1) * 2 * elemsize;
        end = hex_string.size() - i * 2 * elemsize;
      } else {
        start = 0;
        end = hex_string.size();
      }
      std::bitset<256> value = stringToBitset(hex_string.substr(start, end - start));
      m.data_vec = value;
      m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx + i;
      bridge_->process_dut_mcm_read(m_mcmi_read.hart, m, cacheable);
    }
  } else {
    m.pa = start_addr;
    m.size = size;
    m.data_vec = data_vec;
    m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx;
    bridge_->process_dut_mcm_read(m_mcmi_read.hart, m, cacheable);
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  if (in_reset_)
    return;

  if (patch_access(m_mcmi_insert.addr))
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_insert.order;
  m.cycle = m_mcmi_insert.cycle;
  m.v_ext = m_mcmi_insert.v_ext;
  m.elem_idx = m_mcmi_insert.elem_idx;

  process_memory_access(
      m_mcmi_insert.addr,
      m_mcmi_insert.mask,
      m_mcmi_insert.data_vec,
      [&]() {
        // Single consecutive access
        m.pa = m_mcmi_insert.addr;
        m.size = std::popcount(m_mcmi_insert.mask);
        m.data = m_mcmi_insert.data;
        m.data_vec = m_mcmi_insert.data_vec;
        bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
      },
      [&](uint64_t start_addr, size_t size, const std::bitset<256>& data_vec, const std::string&) {
        // Split access
        m.pa = start_addr;
        m.size = size;
        m.data_vec = data_vec;
        bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
      });
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_bypass.addr))
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_bypass.order;
  m.hart = m_mcmi_bypass.hart;
  m.cycle = m_mcmi_bypass.cycle;
  m.v_ext = m_mcmi_bypass.v_ext;
  m.elem_idx = m_mcmi_bypass.elem_idx;
  m.amo = m_mcmi_bypass.amo;
  m.amo_op = m_mcmi_bypass.amo_op;

  process_memory_access(
      m_mcmi_bypass.addr,
      m_mcmi_bypass.mask,
      m_mcmi_bypass.data_vec,
      [&]() {
        // Single consecutive access
        m.pa = m_mcmi_bypass.addr;
        m.size = std::popcount(m_mcmi_bypass.mask);
        m.data = m_mcmi_bypass.data;
        m.data_vec = extract_bits_as_bitset(m_mcmi_bypass.data_vec, m.size * 8, 0);

        if (m.amo && m.amo_op != SC && FLAGS_emulate_amo_arithmetic) {
          amo_writes_.emplace(m.tag, m);
          return;
        }

        if (m.amo && m.amo_op == SC && sc_failed(m)) {
          sc_bypass_.emplace(m.tag, m);
          return;
        }
        if ((m_mcmi_bypass.attr == 0x1000) && (!m.v_ext)) {
          bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m, true);
          // Setting the Cache flag to true for CBO
        } else {
          bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m, false);
        }
      },
      [&](uint64_t start_addr, size_t size, const std::bitset<256>& data_vec, const std::string&) {
        // Split access
        m.pa = start_addr;
        m.size = size;
        m.data_vec = data_vec;
        m.v_ext = m_mcmi_bypass.v_ext;
        m.elem_idx = m_mcmi_bypass.elem_idx;
        bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m, false);
      });
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write) {
  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_write.addr))
    return;

  mem_cl_t m;
  m.valid = true;
  m.cycle = m_mcmi_write.cycle;
  m.pa = m_mcmi_write.addr;
  m.mask = m_mcmi_write.mask;
  m.data = m_mcmi_write.data;
  m.error = m_mcmi_write.error;

  if (check_axi_error(m.pa & ~0x3f)) {
    m.error = 1;
  }

  bridge_->process_dut_mcm_write(m_mcmi_write.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req) {
  if (terminated_ || in_reset_)
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_ifetch_req.order;
  m.pa = m_mcmi_ifetch_req.addr;
  m.attr = m_mcmi_ifetch_req.attr;

  ifetch_reqs_.emplace(m.tag, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp) {
  if (terminated_ || in_reset_)
    return;

  if (ifetch_reqs_.find(m_mcmi_ifetch_resp.order) == ifetch_reqs_.end()) {
    cvm::log(cvm::ERROR, "Error: [mcmi] Ifetch resp with no matching req - [id={}]\n", m_mcmi_ifetch_resp.order);
  }

  mem_t m;
  m = ifetch_reqs_.at(m_mcmi_ifetch_resp.order);
  m.cycle = m_mcmi_ifetch_resp.cycle;

  // RVDE-17736: Manage fetch/evict signaling for ncio region
  if (is_ncio(m.attr)) {
    auto it = std::find_if(ncio_fetches_.begin(), ncio_fetches_.end(), [&](const mem_t& fetch) { return fetch.pa == m.pa; });
    bool active = std::find_if(active_ncio_fetches_.begin(), active_ncio_fetches_.end(), [&](const mem_t& fetch) { return fetch.pa == m.pa; }) != active_ncio_fetches_.end();
    if (it == ncio_fetches_.end()) {
      // First fetch of this ncio line.
      bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);
      ncio_fetches_.emplace_back(m);
    } else if (!active) {
      // Re-fetch in a new window: ncio bytes are volatile, so evict whisper's stale
      // snapshot and re-read the current bytes instead of reusing the pa-dedup entry.
      process(rv_tester_transactions::cosim::m_mcmi_ievict<>(loc_, m.cycle, m_mcmi_ifetch_resp.hart, it->pa));
      bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);
      *it = m;
    }
    active_ncio_fetches_.emplace_back(m);
  } else {
    bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);
    if (!ncio_fetches_.empty()) {
      ncio_mem_transition_ = true;
    }
  }

  ifetch_reqs_.erase(m_mcmi_ifetch_resp.order);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict) {
  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_ievict.addr))
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_ievict.cycle;
  m.pa = m_mcmi_ievict.addr;

  bridge_->process_dut_mcm_ievict(m_mcmi_ievict.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_devict<>& m_mcmi_devict) {
  if (terminated_ || in_reset_)
    return;

  cvm::log(cvm::FULL, "Remote Procedural Call to Whisper for mcm devict to addr : {:#x}\n", m_mcmi_devict.addr);
  bool valid = false;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmDEvictRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), m_mcmi_devict.hart, m_mcmi_devict.cycle, m_mcmi_devict.addr, valid) || !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm devict for address : {:#x} , cycle : {}\n", m_mcmi_devict.hart, m_mcmi_devict.addr, m_mcmi_devict.cycle);
    return;
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_flush<>& m_mcmi_flush) {
  if (in_reset_)
    return;

  cvm::log(cvm::FULL, "Remote Procedural Calls to Whisper for mcm flush (combination of writeback + evict) to addr : {:#x}\n", m_mcmi_flush.addr);
  bool valid = false;

  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmDWritebackRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), m_mcmi_flush.hart, m_mcmi_flush.cycle, m_mcmi_flush.addr, valid) || !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::HIGH, "Hart: {} No mcm dwriteback for cbo flush , since cacheline not present\n", m_mcmi_flush.hart);
    return;
  }

  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmDEvictRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), m_mcmi_flush.hart, m_mcmi_flush.cycle, m_mcmi_flush.addr, valid) || !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed cbo flush mcm devict for address : {:#x} , cycle : {}\n", m_mcmi_flush.hart, m_mcmi_flush.addr, m_mcmi_flush.cycle);
    return;
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_writeback<>& m_mcmi_writeback) {
  if (in_reset_)
    return;

  cvm::log(cvm::FULL, "Remote Procedural Call to Whisper for mcm writeback [sv implementation] to addr : {:#x}\n", m_mcmi_writeback.addr);

  bool valid = false;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmDWritebackRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, m_mcmi_writeback.cycle, m_mcmi_writeback.addr, valid) || !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Failed mcm dwriteback for address : {:#x} , cycle : {}\n", m_mcmi_writeback.addr, m_mcmi_writeback.cycle);
    return;
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_dfetch<>& m_mcmi_dfetch) {
  if (in_reset_)
    return;

  cvm::log(cvm::FULL, "Remote Procedural Call to Whisper for mcm dfetch [sv implementation] to addr : {:#x}\n", m_mcmi_dfetch.addr);

  bool valid = false;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmDFetchRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, m_mcmi_dfetch.cycle, m_mcmi_dfetch.addr, valid) || !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Failed mcm dfetch for address : {:#x} , cycle : {}\n", m_mcmi_dfetch.addr, m_mcmi_dfetch.cycle);
    return;
  }
}

void mcmi::process_ncio_fetches(const rv_instr_t& instr) {
  if (terminated_ || in_reset_)
    return;

  ncio_fetches_.erase(
      std::remove_if(ncio_fetches_.begin(), ncio_fetches_.end(), [&](const mem_t& fetch) {
        bool evict = std::find(active_ncio_fetches_.begin(), active_ncio_fetches_.end(), fetch) == active_ncio_fetches_.end();
        if (evict)
          process(rv_tester_transactions::cosim::m_mcmi_ievict<>(loc_, instr.cycle, instr.hart, fetch.pa));
        return evict;
      }),
      ncio_fetches_.end());
}

void mcmi::process(const rv_tester::terminate_called&) {
  terminated_ = true;
}

void mcmi::process(const rv_tester::terminate_called_mem_checks&) {
  // Handle terminate called mem checks if needed
}

void mcmi::process(const bridge::error_loc&) {
  // Handle bridge error location if needed
}

bool mcmi::patch_access(uint64_t addr) {
  if (!patch_mode_)
    return false;

  if (addr >= patch_ram_lo && addr < patch_ram_hi)
    return true;

  uint64_t pcontrol0 = 0x42005040; //areddy
  for (int i = 0; i < 8; i++)      // do this for all cores0-8
    if (addr == (pcontrol0 + (i * 0x10000)))
      return true;
  return false;
}

bool mcmi::is_ncio(uint32_t mem_attr) {
  return ((mem_attr & 0x800) != 0) || ((mem_attr & 0x1000) == 0);
}

bool mcmi::check_axi_error(uint64_t addr) {
  // Check if the address is expected to have error response by calling AXI instances
  auto type = (FLAGS_vip && !FLAGS_vip_axi_dpi) ? "VIP_AXI" : "AXI";

  // Check all AXI instances
  for (const auto& loc : cvm::topology::get_from_type(type)) {
    uint64_t count = 0;
    if (cvm::registry::messenger.call<axi::check_error_rpc>(loc, addr, count) && count > 0) {
      cvm::log(cvm::HIGH, "[mcmi] check_axi_error: addr={:#x} has error response configured, count={}\n", addr, count);
      return true;
    }
  }

  // Check NCIO_AXI instances
  for (const auto& loc : cvm::topology::get_from_type("NCIO_AXI")) {
    uint64_t count = 0;
    if (cvm::registry::messenger.call<axi::check_error_rpc>(loc, addr, count) && count > 0) {
      cvm::log(cvm::HIGH, "[mcmi] check_axi_error: addr={:#x} has error response configured (NCIO_AXI), count={}\n", addr, count);
      return true;
    }
  }

  return false;
}

std::bitset<256> mcmi::extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb) {
  std::bitset<256> result;

  size_t width = msb - lsb;
  for (size_t i = 0; i < width; ++i) {
    result[i] = bitset[lsb + i];
  }

  return result;
}

bool mcmi::sc_failed(mem_t& write) {
  if (sc_result_.find(write.tag) == sc_result_.end()) {
    return true;
  }

  mem_t m = sc_result_.at(write.tag);

  if (m.data == 0x0) {
    return false;
  }

  return true;
}

void mcmi::process_amo(mem_t& read) {

  if (amo_writes_.find(read.tag) == amo_writes_.end()) {
    cvm::log(cvm::ERROR, "Error: [mcmi] Amo read with no matching bypass write - inst tag={}\n", read.tag);
    return;
  }

  mem_t m = amo_writes_.at(read.tag);
  m.cycle = read.cycle;
  amo_modify_write_data(static_cast<amo_op>(m.amo_op), read.data, m.data, m.size);

  bridge_->process_dut_mcm_bypass(m.hart, m, true);
  amo_writes_.erase(read.tag);
}

void mcmi::amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {

  bool sign = (op == AMOADD) || (op == AMOSWAP) || (op == AMOMAX) || (op == AMOMIN);
  switch (size) {
  case 1:
    if (sign)
      amo_arithmetic<int8_t>(op, read_data, write_data, size);
    else
      amo_arithmetic<uint8_t>(op, read_data, write_data, size);
    break;
  case 2:
    if (sign)
      amo_arithmetic<int16_t>(op, read_data, write_data, size);
    else
      amo_arithmetic<uint16_t>(op, read_data, write_data, size);
    break;
  case 4:
    if (sign)
      amo_arithmetic<int32_t>(op, read_data, write_data, size);
    else
      amo_arithmetic<uint32_t>(op, read_data, write_data, size);
    break;
  case 8:
    if (sign)
      amo_arithmetic<int64_t>(op, read_data, write_data, size);
    else
      amo_arithmetic<uint64_t>(op, read_data, write_data, size);
    break;
  default:
    cvm::log(cvm::ERROR, "Error: [mcmi] Invalid amo op size - [op={}, size={}]\n", amo_op_to_string.at(op), size);
    break;
  }
}

template <typename T>
void mcmi::amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {
  T read = 0, write = 0;

  read = T(read_data) & cvm::bitmanip::mask<T>(size * 8);
  write = T(write_data) & cvm::bitmanip::mask<T>(size * 8);

  T result = 0;

  switch (op) {
  case AMOADD:
    result = read + write;
    break;
  case AMOAND:
    result = read & write;
    break;
  case AMOOR:
    result = read | write;
    break;
  case AMOXOR:
    result = read ^ write;
    break;
  case AMOSWAP:
    result = write;
    break;
  case AMOMIN:
  case AMOMINU:
    result = std::min(read, write);
    break;
  case AMOMAX:
  case AMOMAXU:
    result = std::max(read, write);
    break;
  default:
    assert(false && "Error: [mcmi] Unknown amo operation");
    break;
  }

  write_data = uint64_t(uint64_t(result) & cvm::bitmanip::mask<uint64_t>(size * 8));
}
