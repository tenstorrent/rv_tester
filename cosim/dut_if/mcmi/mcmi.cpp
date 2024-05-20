#include "mcmi.h"
#include "util.h"
#include "whisper_decoder.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"

#include <iostream>
#include <chrono>
#include <cmath>
#include <sstream>

DEFINE_bool(mcm, false, "Enable mcm");
DECLARE_bool(cosim);
DEFINE_bool(emulate_amo_arithmetic, true, "Emulate amo arithmetic if dut harness does not provide amo outputs");

REGISTRY_register(mcmi, COSIM, cvm::registry::all);

mcmi::mcmi(cvm::topology::loc_t loc, unsigned id)
  : loc_(loc), id_(id) {
  init();

  whisper::initialize();

  cvm::registry::messenger.connect<svScope>(
    loc_,
    [&](svScope s) { return this->set_scope(s); });

  connect<
    rv_tester_transactions::cosim::m_mcmi_read<>,
    rv_tester_transactions::cosim::m_mcmi_insert<>,
    rv_tester_transactions::cosim::m_mcmi_write<>,
    rv_tester_transactions::cosim::m_mcmi_bypass<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_req<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>,
    rv_tester_transactions::cosim::m_mcmi_ievict<>,
    rv_tester_transactions::cosim::m_ncio_axi_wr_req<>
  >(loc);

  connect<
    rv_tester::terminate_called
  >(cvm::topology::get_from_type("PLATFORM", 0));
}

mcmi::~mcmi() {
}

void mcmi::init() {
  if (FLAGS_cosim) {
    auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
    bridge_ = std::make_unique<bridge>(cvm::topology::attr(platform_loc, "NHARTS").second, xlen, vlen, loc_, id_);
    bridge_->reset();
  }

}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read) {

  mem_t m;
  m.valid  = true;
  m.hart   = m_mcmi_read.hart;
  m.cycle  = m_mcmi_read.cycle;
  m.tag    = m_mcmi_read.order;
  m.pa     = m_mcmi_read.addr;
  m.size   = std::popcount(m_mcmi_read.mask);
  m.data   = m_mcmi_read.data;
  m.amo    = m_mcmi_read.amo;
  m.amo_op = m_mcmi_read.amo_op;

  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  // Handle SC
  // If read before bypass, store pass/fail result
  // If bypass before read, check pass/fail result and send/don't send bypass
  if (m.amo && m.amo_op == SC) {
    if (sc_bypass_.find(m.tag) == sc_bypass_.end()) {
      sc_result_.emplace(m.tag, m);
    } else {
      if (!sc_failed(sc_bypass_.at(m.tag))) {
        bridge_->process_dut_mcm_bypass(m.hart, sc_bypass_.at(m.tag));
        sc_bypass_.erase(m.tag);
      }
    }
    return;
  }

  bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);

  if (m.amo && m.amo_op != LR && FLAGS_emulate_amo_arithmetic) {
    process_amo(m);
  }
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_insert.cycle;
  m.tag   = m_mcmi_insert.order;
  m.pa    = m_mcmi_insert.addr;
  m.size  = std::popcount(m_mcmi_insert.mask);
  m.data  = m_mcmi_insert.data;

  bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid  = true;
  m.hart   = m_mcmi_bypass.hart;
  m.cycle  = m_mcmi_bypass.cycle;
  m.tag    = m_mcmi_bypass.order;
  m.pa     = m_mcmi_bypass.addr;
  m.size   = std::popcount(m_mcmi_bypass.mask);
  m.data   = m_mcmi_bypass.data;
  m.amo    = m_mcmi_bypass.amo;
  m.amo_op = m_mcmi_bypass.amo_op;

  if (m.amo && m.amo_op != SC && FLAGS_emulate_amo_arithmetic) {
    amo_writes_.emplace(m.tag, m);
    return;
  }

  if (m.amo && m.amo_op == SC && sc_failed(m)) {
    sc_bypass_.emplace(m.tag, m);
    return;
  }

  if (!m.amo){
    mcmi_bypass_ncio_map_.emplace(m.pa, m);
    return;
  }

  bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_cl_t m;
  m.valid = true;
  m.cycle = m_mcmi_write.cycle;
  m.pa = m_mcmi_write.addr;
  m.mask = m_mcmi_write.mask;
  m.data = m_mcmi_write.data;

  bridge_->process_dut_mcm_write(m_mcmi_write.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_ifetch_req.order;
  m.pa = m_mcmi_ifetch_req.addr;

  ifetch_reqs_.emplace(m.tag, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  if (ifetch_reqs_.find(m_mcmi_ifetch_resp.order) == ifetch_reqs_.end()) {
    cvm::log(cvm::ERROR, "Error: Ifetch resp with no matching req - [id={}]", m_mcmi_ifetch_resp.order);
  }

  mem_t m;
  m = ifetch_reqs_.at(m_mcmi_ifetch_resp.order);
  m.cycle = m_mcmi_ifetch_resp.cycle;

  bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);

  ifetch_reqs_.erase(m_mcmi_ifetch_resp.order);
}

void mcmi::process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;
    
  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_ievict.cycle;
  m.pa = m_mcmi_ievict.addr;

  bridge_->process_dut_mcm_ievict(m_mcmi_ievict.hart, m);
}

void mcmi::process(const rv_tester_transactions::cosim::m_ncio_axi_wr_req<>& m_ncio_axi_wr_req) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  if (mcmi_bypass_ncio_map_.find(m_ncio_axi_wr_req.addr) != mcmi_bypass_ncio_map_.end()) {
    auto m = mcmi_bypass_ncio_map_.at(m_ncio_axi_wr_req.addr);
    m.cycle = m_ncio_axi_wr_req.cycle;
    bridge_->process_dut_mcm_bypass(m.hart, m);
    mcmi_bypass_ncio_map_.erase(m_ncio_axi_wr_req.addr);
  }

}

void mcmi::process(const rv_tester::terminate_called&) {
  terminated_ = true;
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
    cvm::log(cvm::ERROR, "Error: Amo read with no matching bypass write - inst tag={}\n", read.tag);
    return;
  }

  mem_t m = amo_writes_.at(read.tag);
  m.cycle = read.cycle;
  amo_modify_write_data(static_cast<amo_op>(m.amo_op), read.data, m.data, m.size);

  bridge_->process_dut_mcm_bypass(m.hart, m);

  amo_writes_.erase(read.tag);
}

void mcmi::amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {

  bool sign = (op == AMOADD) || (op == AMOSWAP) || (op == AMOMAX) || (op == AMOMIN);
  switch (size) {
    case 1:
      if (sign) amo_arithmetic<int8_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint8_t>(op, read_data, write_data, size);
      break;
    case 2:
      if (sign) amo_arithmetic<int16_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint16_t>(op, read_data, write_data, size);
      break;
    case 4:
      if (sign) amo_arithmetic<int32_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint32_t>(op, read_data, write_data, size);
      break;
    case 8:
      if (sign) amo_arithmetic<int64_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint64_t>(op, read_data, write_data, size);
      break;
    default:
      cvm::log(cvm::ERROR, "Error: Invalid amo op size - [op={}, size={}]\n", amo_op_to_string.at(op), size);
      break;
  }
}

template <typename T>
void mcmi::amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {
  T read = 0, write = 0;

  read = T(read_data) & cvm::bitmanip::mask<T>(size*8);
  write = T(write_data) & cvm::bitmanip::mask<T>(size*8);

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
      assert(false && "Error: Unknown amo operation");
      break;
  }

  write_data = uint64_t(uint64_t(result) & cvm::bitmanip::mask<uint64_t>(size*8));
}

void mcmi::check(){
  if (!mcmi_bypass_ncio_map_.empty()) {
    std::string errorMsg;
    errorMsg = "Error: MCMI bypass NCIO map not empty, addresses are:";
    for (auto& m : mcmi_bypass_ncio_map_) {
      uint64_t value = m.first;
      std::stringstream ss;
      ss << std::hex << value;
      errorMsg.append("\t").append("0x").append(ss.str());
    }
    cvm::log(cvm::ERROR, errorMsg);
  }
}