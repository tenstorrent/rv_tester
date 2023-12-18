#include "eot.h"
#include <chrono>
#include <iostream>
#include "common/memmap.h"
#include "sysmod/htif/htif.h"

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr");
DEFINE_uint64(tohost, 0x0, "Use this tohost address if provided");
DEFINE_uint32(max_instr, 100000, "Max instruction limit to terminate the sim");
DECLARE_string(load);
DECLARE_string(hex);


void eot::get_tohost_addr() {

  // Get tohost address from
  // 1. plusarg if provided
  if (FLAGS_tohost != 0x0) {
    tohost_addr_ = FLAGS_tohost;
    cvm::log(cvm::NONE, "[eot] tohost from plusarg:: addr=[{:#x}]\n", tohost_addr_);
    return;
  }

  // 2. elf if available
  bool tohost_in_elf = false;
  if (FLAGS_load != "" && FLAGS_hex == "") {
    std::string cmd = "nm " + FLAGS_load + " | grep -w tohost";
    std::string result = cosim_util::exec(cmd.c_str());
    std::string addr_str = result.substr(0, 16);
    tohost_in_elf = true;

    try {
      tohost_addr_ = std::stoul(addr_str, nullptr, 16);
    }
    catch (...) {
      tohost_in_elf = false;
      if (FLAGS_eot == "tohost") {
        cvm::log(cvm::NONE, "Warn: No tohost symbol in elf\n");
      }
    }
    cvm::log(cvm::NONE, "[eot] tohost from elf:: cmd=[{}] addr_str=[{}] addr=[{:#x}]\n", cmd, addr_str, tohost_addr_);
  }
  if (tohost_in_elf)
    return;

  // 3. htif address from memmap
  memmap::memmap_t m;
  memmap::get(m);
  if (m.count("htif") > 0) {
    tohost_addr_ = m.at("htif").base;
    cvm::log(cvm::NONE, "[eot] tohost from memmap:: addr=[{:#x}]\n", tohost_addr_);
  } else {
    cvm::log(cvm::ERROR, "[eot] tohost from memmap:: htif not found in memmap\n", tohost_addr_);
  }
}

void eot::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (ended_)
      return;

  instr_count_++;

  // End test on max_instr
  if (FLAGS_max_instr > 0 && instr_count_ > FLAGS_max_instr) {
    ended_ = true;
    if (FLAGS_eot == "max_instr") {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      cvm::log(cvm::NONE, "<{}> Pass condition detected: +eot=max_instr +max_instr={}\n", m_rvfi.cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      auto location = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
      cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{});
      auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      cvm::log(cvm::HIGH, "end time: {}\n", std::ctime(&now));
      return;
    } else {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      cvm::log(cvm::ERROR, "Error: max_instr limit reached: {}\n", m_rvfi.cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      return;
    }
  }
}

void eot::process_tohost(std::tuple<uint64_t,uint64_t,uint64_t> w) {

  if (ended_)
      return;

  if (tohost_addr_ != std::get<1>(w))
    return;

  if (tohost_status_ != (std::get<2>(w) & 0x1))
    return;

  if (tohost_device_syscall_ != ((std::get<2>(w) >> 56) & 0xff))
    return;

  uint64_t cycle = std::get<0>(w);
  uint64_t exit_code = (std::get<2>(w) >> 1) & 0x7fffffffffff;


  ended_ = true;

  if (exit_code == 0) {
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::NONE, "<{}> Pass condition detected - tohost[0]=1, tohost[47:1]=0\n", cycle);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    auto location = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{});
  } else {
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::ERROR, "<{}> Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle,
      exit_code);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  }
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  process_tohost(std::make_tuple(m_mcmi_insert.cycle, m_mcmi_insert.addr, m_mcmi_insert.data));
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
  process_tohost(std::make_tuple(m_mcmi_bypass.cycle, m_mcmi_bypass.addr, m_mcmi_bypass.data));
}

