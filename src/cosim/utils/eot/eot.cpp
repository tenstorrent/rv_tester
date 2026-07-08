#include "eot.h"
#include "svdpi.h"
#include <chrono>
#include <iostream>
#include "common/memmap.h"
#include "sysmod_plusargs.h"
#include "whisper_client_plusargs.h"
#include "rvfi_plusargs.h"
#include "rv_tester_plusargs.h"
#include "src/sysmod/sysmod_rpc.h"
#include "rv_tester_structs.h"

extern "C" void rv_tester_set_eot_addr(std::uint64_t addr);

constexpr std::uint64_t recent_pc_default = std::numeric_limits<std::uint64_t>::max();

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr, tohost_all");
DEFINE_uint64(tohost, 0x0, "Use this tohost address if provided");
DEFINE_uint64(max_instr, 100000, "Max instruction limit to terminate the sim");
DEFINE_uint64(min_instr, 0, "min instruction limit to pass the sim");
DEFINE_uint64(recent_pc, recent_pc_default, "The PC that must be in the last +recent_pc_instr instructions before the test ended");
DEFINE_uint64(recent_pc_instr, 100000, "+recent_pc should have been seen within this many instructions of end of test");
DEFINE_uint64(psc_off_high, 0, "Turn Period-Cosim mode BACK ON when clocks > psc_off_high");
DEFINE_uint64(psc_off_low, 0, "Turn Period-Cosim mode OFF     when clocks > psc_off_low");
DEFINE_bool(hw_eot_enable, false, "Enable hardware termination of the EOT (useful for offline DPI testing when rvfi is not sent to cosim)");
DEFINE_bool(eot_mem_check, false, "Do End of Test memory checks");
REGISTRY_register(eot, TOP.PLATFORM, cvm::registry::all);

eot::eot(cvm::topology::loc_t loc, unsigned id) : id_(id), loc_(loc) {

  for (uint32_t i = 0; i < num_harts_; i++) {
    instr_count_.push_back(0);
  }
}

void eot::configure() {
  for (uint32_t i = 0; i < num_harts_; i++) {
    connect<
        rv_tester_transactions::cosim::m_rvfi<>,
        rv_tester_transactions::cosim::m_steps<>,
        rv_tester_transactions::cosim::m_eoti_normal<>,
        rv_tester_transactions::cosim::offline_eoti<>>(cvm::topology::get_from_type("COSIM", i));
  }
  cvm::registry::messenger.procedure<get_tohost_addr_RPC>(loc_, [this]() { return this->get_tohost_addr(); });
  cvm::registry::messenger.connect<rv_tester::snoop_mem>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SNOOP_GEN", 0), [this](rv_tester::snoop_mem) {
    bool passed = this->mem_checks();
    eot_mem_checks_done_ = true;
    this->eot_terminate(passed);
  });
  cvm::registry::messenger.procedure<init_tohost_addr_RPC>(loc_, [this]() { this->init_tohost_addr(); });

  init_tohost_addr();
}

void eot::init_tohost_addr() {
  resolve_tohost_addr();
  cvm::registry::callbacks.push(loc_, [&]() { rv_tester_set_eot_addr(tohost_addr_); });
}

void eot::resolve_tohost_addr() {

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
      FLAGS_tohost = tohost_addr_;
    } catch (...) {
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
  if (!memmap::instance().get(m))
    cvm::log(cvm::ERROR, "Error: Unable to get memmap\n");

  if (m.count("htif") > 0) {
    tohost_addr_ = m.at("htif").base;
    FLAGS_tohost = tohost_addr_;
    cvm::log(cvm::NONE, "[eot] tohost from memmap:: addr=[{:#x}]\n", tohost_addr_);
  } else {
    cvm::log(cvm::ERROR, "Error: [eot] tohost from memmap:: htif not found in memmap\n", tohost_addr_);
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
    recent_pc_hart_ = m_rvfi.hart;
  }
  //for (uint32_t i = 0; i < num_harts_; i++) {
  //    check_max_instr(m_rvfi.cycle,instr_count_[i]);
  //}
}

void eot::process_eoti(uint64_t hartid, uint64_t cycle, uint64_t instr_count, uint64_t data, int max_instr) {
  if (ended_)
    return;

  if (max_instr == 1) {
    if (FLAGS_max_instr > 0 && instr_count > FLAGS_max_instr) {
      ended_ = true;
      if (FLAGS_eot == "max_instr") {
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
        cvm::log(cvm::NONE, "<{}> Hart:<{}> Pass condition detected: +eot=max_instr +max_instr={}\n", cycle, hartid, FLAGS_max_instr);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
        eot_terminate(true);
        return;
      } else {
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
        cvm::log(cvm::ERROR, "<{}> Error: Hart:<{}> max_instr limit reached: {}\n", cycle, hartid, FLAGS_max_instr);
        cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
        eot_terminate(true);
        return;
      }
    }
  }

  uint64_t exit_code = (data >> 1) & 0x7fffffffffff;

  end = std::chrono::system_clock::now();

  if (exit_code == 0) {
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
    cvm::log(cvm::ERROR, "<{}> Hart:<{}>Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle, hartid, exit_code);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    eot_terminate(false);
  }
}

//---------------------------------------------------------------------
// ONLINE DPI EOT calls (sent in normal/offline-replay)
//---------------------------------------------------------------------
void eot::process(const rv_tester_transactions::cosim::m_eoti_normal<>& m_eoti_normal) {
  int max_instr = 0;
  if (m_eoti_normal.max_instr) {
    max_instr = 1;
  }
  process_eoti(m_eoti_normal.hart, m_eoti_normal.cycle, m_eoti_normal.icount, m_eoti_normal.data, max_instr);
}

void eot::process(const rv_tester_transactions::cosim::offline_eoti<>& offline_eoti) {
  int max_instr = 0;
  if (offline_eoti.max_instr) {
    max_instr = 1;
  }
  process_eoti(offline_eoti.hart, offline_eoti.cycle, offline_eoti.icount, offline_eoti.data, max_instr);
}

void eot::eot_terminate(bool passed) {
  bool terminate = false;
  if (!(FLAGS_offline_dpi | FLAGS_offline_dpi_test)) {
    FLAGS_eot_mem_check = FLAGS_eot_mem_check & FLAGS_mcm;
    if (!FLAGS_eot_mem_check || !passed || eot_mem_checks_done_)
      terminate = true;
    else if (FLAGS_eot_mem_check && !eot_mem_checks_done_)
      mem_checks_snoop();
  } else {
    terminate = true;
  }
  if (terminate)
    cvm::registry::messenger.signal<htif::terminate_t>(loc_, htif::terminate_t{.low_priority_based = true});
}

void eot::mem_checks_snoop() {
  std::map<std::string, memmap_entry_t> m;
  if (!memmap::instance().get(m))
    cvm::log(cvm::ERROR, "Error: Unable to get memmap\n");
  uint64_t mem_base = m.at("memory").base;
  uint64_t mem_size = m.at("memory").size;
  uint64_t secure_mem_region_start;
  uint64_t secure_mem_region_end;
  cvm::registry::messenger.call<sysmod_secure_mem>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), secure_mem_region_start, secure_mem_region_end);
  // RPC to get secure region bounds

  cvm::log(cvm::MEDIUM, "Initiating EOT MEMORY CHECKS\n");
  cvm::log(cvm::HIGH, "Secure region start: {:#x}, Secure region end: {:#x}\n", secure_mem_region_start, secure_mem_region_end);
  cvm::registry::messenger.signal_async<rv_tester::terminate_called_mem_checks>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called_mem_checks{});
  std::ifstream file(FLAGS_whisper_data_lines);
  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string token;
    uint64_t addr;
    std::bitset<512> value;
    bool ignore_addr = false;
    int count = 0;
    try {
      while (std::getline(ss, token, ':')) {
        switch (count++) {
        case 0:
          break;
        case 1:
          addr = std::stoul(token, nullptr, 16);
          addr = addr * 64;
          if (addr >= secure_mem_region_start && addr < secure_mem_region_end) {
            addr = addr | (uint64_t(1) << 55); // set the secure bit
          }
          if (!(addr >= mem_base && addr < (mem_base + mem_size)))
            ignore_addr = true;
          break;
        case 2:
          if (ignore_addr)
            break;
          value = std::bitset<512>(cosim_util::hex_string_to_binary_string(token));
          mem_lines_.push_back({addr, value});
        default:
          break;
        }
      }
    } catch (...) {
      cvm::log(cvm::ERROR, "Error: parsing whisper token:{}\n", token);
    }
  }
  if (mem_lines_.size() == 0) {
    cvm::log(cvm::HIGH, "Done with Memory Checks: No Whisper lines\n");
    eot_mem_checks_done_ = true;
    eot_terminate(true);
    return;
  }
  rv_tester::snoop_addrs_eot addrs_eot;
  for (auto& w : mem_lines_) {
    cvm::log(cvm::HIGH, "Snoop Address: {:#x}\n", w.first);
    addrs_eot.address.push(w.first);
  }
  cvm::registry::messenger.signal<rv_tester::snoop_addrs_eot>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), addrs_eot);
}

bool eot::mem_checks() {
  // The data from Caches are flushed out and written back to memory of Sysmod
  // Begin Actual Checking
  auto vec_to_bitset = [](std::vector<uint8_t> t) {
    std::bitset<512> value;
    std::reverse(t.begin(), t.end());
    for (const auto& i : t) {
      value = value << 8;
      std::bitset<512> val2(i);
      value |= val2;
    }
    return value;
  };
  auto bitset_to_hex_str = [](std::bitset<512> t) {
    std::string ret = "";
    while (t.any()) {
      std::bitset<512> tmp = t & std::bitset<512>(0xffffffffffffffff);
      std::ostringstream ss;
      ss << std::setw(16) << std::setfill('0') << std::hex << tmp.to_ulong();
      std::string s = ss.str();
      ret = s + ret;
      t >>= 64;
    }
    if (ret == "")
      ret = "0";
    return "0x" + ret;
  };
  auto first_differing_addr = [](std::bitset<512> a, std::bitset<512> b) {
    std::bitset<512> diff = a ^ b;
    int i = 0;
    while (!diff[i++])
      ;
    return --i / 8;
  };
  cvm::log(cvm::HIGH, "[EOT] Memory Checks: Actual Check\n");
  eot_mem_checks_done_ = true;
  for (auto& w : mem_lines_) {
    device::data_t data(64);
    cvm::registry::messenger.call<sysmod_eot>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), w.first, 64, data);
    auto dut = vec_to_bitset(data);
    if (!(dut == w.second)) {
      cvm::log(cvm::ERROR, "Error: [EOT] Memory Mismatch Addr: 0x{:x}\nDUT:{}\nISS:{}\n", w.first + first_differing_addr(dut, w.second), bitset_to_hex_str(dut), bitset_to_hex_str(w.second));
      return false;
    } else {
      cvm::log(cvm::HIGH, "[EOT] Memory Check: Data at Addr: 0x{:x} consistent\n", w.first);
    }
  }
  return true;
}

eot::~eot() {
  int h = 0;
  //----------------------------------------------------------------
  // during offline_dpi we cannot do these checks so just return
  //----------------------------------------------------------------
  if (FLAGS_offline_dpi | FLAGS_offline_dpi_test)
    return;
  for (const auto i : instr_count_) {
    if (i < FLAGS_min_instr) {
      cvm::log(cvm::ERROR, "Error: Hart:<{}> instruction count {} did not meet +min_instr={}\n", h, i, FLAGS_min_instr);
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
int is_eot_tohost() {
  if (FLAGS_eot == "tohost")
    return 1;
  if (FLAGS_eot == "tohost_all")
    return 1;
  return 0;
}
}
