#include "eot.h"
#include "sysmod/htif/htif.h"

DEFINE_string(eot, "tohost", "Enable end-of-test mechanism. Supported options: tohost, max_instr");
DECLARE_string(load);
DECLARE_bool(terminate_call_finish);

void eot::get_tohost_addr() {

  std::string cmd = "nm " + FLAGS_load + " | grep -w tohost";
  std::string result = cosim_util::exec(cmd.c_str());
  std::string addr_str = result.substr(0, 16);

  try {
    tohost_addr_ = std::stoul(addr_str, nullptr, 16);
  }
  catch (...) {
    if (FLAGS_eot == "tohost") {
      cvm::log(cvm::NONE, "Error: No tohost symbol in elf\n");
    }
  }

  cvm::log(cvm::NONE, "eot::get_tohost_addr:: cmd=[{}] addr_str=[{}] addr=[{:#x}]\n", cmd, addr_str, tohost_addr_);

}

void eot::process(const cosim_transactions::m_mcmi_store& m_mcmi_store) {

  if (tohost_addr_ != m_mcmi_store.addr)
    return;

  if (tohost_status_ != (m_mcmi_store.data & 0x1))
    return;

  if (tohost_device_syscall_ != ((m_mcmi_store.data >> 56) & 0xff))
    return;

  uint64_t cycle = m_mcmi_store.cycle;
  uint64_t exit_code = (m_mcmi_store.data >> 1) & 0x7fffffffffff;
  
  if (exit_code == 0) {
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::NONE, "<{}> Pass condition detected - tohost[0]=1, tohost[47:1]=0\n", cycle);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    auto location = cvm::topology::get("platform", 0);
    cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{FLAGS_terminate_call_finish});
  } else {
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    cvm::log(cvm::NONE, "<{}> Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle, 
      exit_code);
    cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
    auto location = cvm::topology::get("platform", 0);
    cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{FLAGS_terminate_call_finish});
  }
}
