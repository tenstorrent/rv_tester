#include "eot.h"
#include "sysmod/htif/htif.h"
#include <chrono>
#include <iostream>

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr");
DEFINE_uint32(max_instr, 100000, "Max instruction limit to terminate the sim");
DECLARE_string(load);

void eot::get_tohost_addr() {

  std::string cmd = "nm " + FLAGS_load + " | grep -w tohost";
  std::string result = cosim_util::exec(cmd.c_str());
  std::string addr_str = result.substr(0, 16);

  try {
    tohost_addr_ = std::stoul(addr_str, nullptr, 16);
  }
  catch (...) {
    if (FLAGS_eot == "tohost") {
      cvm::log(cvm::ERROR, "Error: No tohost symbol in elf\n");
    }
  }

  cvm::log(cvm::NONE, "eot::get_tohost_addr:: cmd=[{}] addr_str=[{}] addr=[{:#x}]\n", cmd, addr_str, tohost_addr_);

}

void eot::process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi) {

  if (ended_)
      return;

  instr_count_++;

  // End test on max_instr
  if (FLAGS_max_instr > 0 && instr_count_ > FLAGS_max_instr) {
    ended_ = true;
    if (FLAGS_eot == "max_instr") {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      cvm::log(cvm::NONE, "<{}> Stop condition detected: +eot=max_instr +max_instr={}\n", m_rvfi.cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      auto location = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
      cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{});
      auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::cout << "end time: " << std::ctime(&now) << std::endl;
      return;
    } else {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      cvm::log(cvm::ERROR, "Error: max_instr limit reached: {}\n", m_rvfi.cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", m_rvfi.cycle);
      return;
    }
  }
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_insert& m_mcmi_insert) {

  if (ended_)
      return;

  if (tohost_addr_ != m_mcmi_insert.addr)
    return;

  if (tohost_status_ != (m_mcmi_insert.data & 0x1))
    return;

  if (tohost_device_syscall_ != ((m_mcmi_insert.data >> 56) & 0xff))
    return;

  uint64_t cycle = m_mcmi_insert.cycle;
  uint64_t exit_code = (m_mcmi_insert.data >> 1) & 0x7fffffffffff;


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
