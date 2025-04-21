#include "eot.h"
#include "svdpi.h"
#include <chrono>
#include <iostream>
#include "common/memmap.h"
#include "sysmod/sysmod_plusargs.h"

constexpr std::uint64_t recent_pc_default = std::numeric_limits<std::uint64_t>::max();

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr, tohost_all");
DEFINE_uint64(tohost, 0x0, "Use this tohost address if provided");
DEFINE_uint64(max_instr, 100000, "Max instruction limit to terminate the sim");
DEFINE_uint64(min_instr,      0, "min instruction limit to pass the sim");
DEFINE_uint64(recent_pc, recent_pc_default, "The PC that must be in the last +recent_pc_instr instructions before the test ended");
DEFINE_uint64(recent_pc_instr, 100000, "+recent_pc should have been seen within this many instructions of end of test");
DEFINE_uint64(psc_off_high, 0, "Turn Period-Cosim mode BACK ON when clocks > psc_off_high");
DEFINE_uint64(psc_off_low,  0, "Turn Period-Cosim mode OFF     when clocks > psc_off_low");
DEFINE_bool(hw_eot_enable,  true, "Enable hardware termination of the EOT (useful for offline DPI testing when rvfi is not sent to cosim)");

REGISTRY_register(eot, TOP.PLATFORM, cvm::registry::all);

void eot::init_tohost_addr() {

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
      if (FLAGS_eot == "tohost" || FLAGS_eot == "tohost_all") {
        cvm::log(cvm::NONE, "Warn: No tohost symbol in elf\n");
      }
    }
    cvm::log(cvm::NONE, "[eot] tohost from elf:: cmd=[{}] addr_str=[{}] addr=[{:#x}]\n", cmd, addr_str, tohost_addr_);
  }
  if (tohost_in_elf)
    return;

  // 3. htif address from memmap
  std::map<std::string, memmap_entry_t> m;
  if(!cvm::registry::messenger.call<memmap::getRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.MEMMAP", 0), m))
      cvm::log(cvm::ERROR, "Unable to get memmap\n");

  if (m.count("htif") > 0) {
    tohost_addr_ = m.at("htif").base;
    cvm::log(cvm::NONE, "[eot] tohost from memmap:: addr=[{:#x}]\n", tohost_addr_);
  } else {
    cvm::log(cvm::ERROR, "[eot] tohost from memmap:: htif not found in memmap\n", tohost_addr_);
  }
}

std::uint64_t eot::get_tohost_addr() {
   return tohost_addr_;
}


void eot::process(const rv_tester_transactions::cosim::m_steps<>& m_steps) {

  // When using periodic state check method add the missing step counts (only the first m_steps packet does this)
  if (m_steps.steps > 0) {
      instr_count_[m_steps.hart] = instr_count_[m_steps.hart] + m_steps.steps + m_steps.final_steps;
      previous_cycle_ = m_steps.cycle;
  }
}

void eot::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (ended_)
      return;

  if (instr_count_[m_rvfi.hart] == 0) {
     start = std::chrono::system_clock::now();
  }

  // We don't want to increment instr_count and check for EOT if this is not the last uop
  if (!m_rvfi.last_uop)
      return;

  instr_count_[m_rvfi.hart]++;

  if (m_rvfi.pc_rdata == FLAGS_recent_pc) {
      recent_pc_instr_count_ = instr_count_[m_rvfi.hart];
      recent_pc_hart_        = m_rvfi.hart;
  }
  if (!FLAGS_hw_eot_enable) {
    for (uint32_t i = 0; i < num_harts_; i++) {
      check_max_instr(m_rvfi.cycle,instr_count_[i]);
    }
  }
}

void eot::check_max_instr(std::uint64_t cycle, std::uint64_t instr_count)
{
  if (ended_)
     return;
  // End test on max_instr
  if (FLAGS_max_instr > 0 && instr_count > FLAGS_max_instr) {
    ended_ = true;
    end = std::chrono::system_clock::now();
    if (FLAGS_eot == "max_instr") {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
      cvm::log(cvm::NONE, "<{}> Pass condition detected: +eot=max_instr +max_instr={}\n", cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
      eot_terminate(true);
      return;
    } else {
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
      cvm::log(cvm::ERROR, "<{}> Error: max_instr limit reached: {}\n", cycle, FLAGS_max_instr);
      cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n",cycle);
      eot_terminate(true);
      return;
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
      if (FLAGS_eot != "tohost_all" || (terminated_harts_.size() >= FLAGS_num_harts)) {
        ended_ = true;
        eot_terminate(true);
      }
  } else {
    ended_ = true;
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::ERROR, "<{}> Hart:<{}>Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle,
      hartid, exit_code);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    eot_terminate(false);
  }
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
   if (!FLAGS_hw_eot_enable) {
     process_tohost(m_mcmi_insert.hart, m_mcmi_insert.cycle, m_mcmi_insert.addr, m_mcmi_insert.data);
   }
}

void eot::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
   if (!FLAGS_hw_eot_enable) {
     process_tohost(m_mcmi_bypass.hart, m_mcmi_bypass.cycle, m_mcmi_bypass.addr, m_mcmi_bypass.data);
   }
}

void eot::eot_terminate(bool) {
  cvm::registry::messenger.signal<htif::terminate_t>(loc_, htif::terminate_t{.low_priority_based = true});
}

eot::~eot() {
    int h = 0;
    for(const auto i : instr_count_) {
        if (i < FLAGS_min_instr) {
            cvm::log(cvm::ERROR, "Hart:<{}> Error: instruction count {} did not meet +min_instr={}\n", h, i, FLAGS_min_instr);
        }
        h++;
    }
    if (FLAGS_recent_pc != recent_pc_default) {
        if (!(recent_pc_hart_ >= 0 && instr_count_[recent_pc_hart_] - recent_pc_instr_count_ <= FLAGS_recent_pc_instr)) {
            cvm::log(cvm::ERROR, "Error: did not see PC +recent_pc=0x{:#x} in +recent_pc_instr={} instructions before test ended\n", FLAGS_recent_pc, FLAGS_recent_pc_instr);
        }
    }
}

extern "C" {
  static bool hw_eot_terminated = false;
  std::uint64_t eot_get_addr() {
    hw_eot_terminated = false;
    return cvm::registry::messenger.call<eot::get_tohost_addr_RPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0));
  }
  void eot_hw_process(std::uint64_t hart, std::uint64_t cycle, std::uint64_t addr, std::uint64_t data) {
     cvm::registry::messenger.call<eot::process_tohost_RPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0),hart, cycle, addr, data);
  }
  void call_check_max_instr(std::uint64_t cycle, std::uint64_t count) {
    if ((FLAGS_eot == "max_instr") && !hw_eot_terminated) {
      if (FLAGS_max_instr > 0 && count > FLAGS_max_instr) {
        cvm::registry::messenger.call<eot::check_max_instr_RPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0),cycle, count);
        hw_eot_terminated = true;
      }
    }
  }
  int is_eot_tohost() {
    if (FLAGS_eot == "tohost")
      return 1;
    if (FLAGS_eot == "tohost_all")
      return 1;
    return 0;
  }
}

