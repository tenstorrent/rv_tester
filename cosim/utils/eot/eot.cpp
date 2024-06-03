#include "eot.h"
#include "svdpi.h"
#include <chrono>
#include <iostream>
#include "common/memmap.h"
#include "sysmod/htif/htif.h"

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr, tohost_all");
DEFINE_uint64(tohost, 0x0, "Use this tohost address if provided");
DEFINE_uint32(max_instr, 100000, "Max instruction limit to terminate the sim");
DECLARE_string(load);
DECLARE_string(hex);

REGISTRY_register(eot, TOP.PLATFORM, cvm::registry::all);

extern "C" void cosim_set_eot(std::uint64_t addr, std::uint8_t status, std::uint8_t syscall);

void eot::get_tohost_addr() {

  // Get tohost address from
  // 1. plusarg if provided
  if (FLAGS_tohost != 0x0) {
    tohost_addr_ = FLAGS_tohost;
    cvm::log(cvm::NONE, "[eot] tohost from plusarg:: addr=[{:#x}]\n", tohost_addr_);

    cosim_set_eot(tohost_addr_,1,0);
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
      if (FLAGS_eot == "tohost" || FLAGS_eot == "tohost_all") {
        cvm::log(cvm::NONE, "Warn: No tohost symbol in elf\n");
      }
    }
    cvm::log(cvm::NONE, "[eot] tohost from elf:: cmd=[{}] addr_str=[{}] addr=[{:#x}]\n", cmd, addr_str, tohost_addr_);
    cosim_set_eot(tohost_addr_,1,0);
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
  cosim_set_eot(tohost_addr_,1,0);
}

void eot::process(const rv_tester_transactions::cosim::m_steps<>& m_steps) {

  // When using periodic state check method add the missing step counts (only the first m_steps packet does this)
  if (m_steps.steps > 0) {
      instr_count_[m_steps.hart] = instr_count_[m_steps.hart] + m_steps.steps;
      previous_cycle_ = m_steps.cycle;
  }
}

void eot::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (ended_)
      return;

  if (instr_count_[m_rvfi.hart] == 0) {
     start = std::chrono::system_clock::now();
  }

  instr_count_[m_rvfi.hart]++;

  // End test on max_instr
  for (uint32_t i = 0; i < num_harts_; i++) {
    if (FLAGS_max_instr > 0 && instr_count_[i] > FLAGS_max_instr) {
      ended_ = true;
      end = std::chrono::system_clock::now();
      if (FLAGS_eot == "max_instr") {
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
        cvm::log(cvm::NONE, "<{}> Pass condition detected: +eot=max_instr +max_instr={}\n", m_rvfi.cycle, FLAGS_max_instr);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
        auto location = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
        cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{.low_priority_based = true});
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        cvm::log(cvm::HIGH, "end time: {}\n", std::ctime(&now));
        return;
      } else {
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
        cvm::log(cvm::ERROR, "<{}> Error: max_instr limit reached: {}\n", m_rvfi.cycle, FLAGS_max_instr);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
        return;
      }
    }
  }
}

void eot::process_tohost(uint64_t hartid, uint64_t cycle, uint64_t address, uint64_t data) {
  if (ended_)
      return;

  if (tohost_addr_ != address)
    return;

  if (tohost_status_ != (data & 0x1))
    return;

  if (tohost_device_syscall_ != ((data >> 56) & 0xff))
    return;

  uint64_t exit_code = (data >> 1) & 0x7fffffffffff;

  end = std::chrono::system_clock::now();

  if (exit_code == 0 ) {
      if (!std::count(terminated_harts_.begin(), terminated_harts_.end(), hartid)) {
        eot::terminated_harts_.emplace_back(hartid);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
        cvm::log(cvm::NONE, "<{}> Hart:<{}> Pass condition detected - tohost[0]=1, tohost[47:1]=0\n", cycle, hartid);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
      }
      if (FLAGS_eot != "tohost_all" || (terminated_harts_.size() >= num_harts_)) {
        ended_ = true;
        cvm::registry::messenger.signal<htif::terminate_t>( cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0),
        htif::terminate_t{.low_priority_based = true});
      }
  } else {
    ended_ = true;
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::ERROR, "<{}> Hart:<{}>Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle,
      hartid, exit_code);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  }
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
   process_tohost(m_mcmi_insert.hart, m_mcmi_insert.cycle, m_mcmi_insert.addr, m_mcmi_insert.data);
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
   process_tohost(m_mcmi_bypass.hart, m_mcmi_bypass.cycle, m_mcmi_bypass.addr, m_mcmi_bypass.data);
}


