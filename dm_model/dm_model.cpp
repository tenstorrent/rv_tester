#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath> 
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "dm_model.hpp"

DEFINE_bool(dm_hart_enum_chk, false, "Check DM hartenumaration wrt VID/PID mapping");
// Return the number of bits wide that a field has to be to encode up to n
// different values.
// 1->0, 2->1, 3->2, 4->2
static unsigned field_width(unsigned n)
{
  unsigned i = 0;
  n -= 1;
  while (n)
  {
    i++;
    n >>= 1;
  }
  return i;
}

bool ndm_reset_assert, hartsel_stable;
uint32_t hart_haltreq_hg, hart_abscmd, hartsel;

REGISTRY_register(debug_module_t, TOP.PLATFORM.DM_MODEL, 0);
DEFINE_bool(dm_model_check_bypass, false, "Bypass the DM Model checks");

debug_module_t::debug_module_t(cvm::topology::loc_t loc, unsigned) : program_buffer_bytes((config.support_impebreak ? 4 + 4 : 0) + (4 * config.progbufsize)),
                                                                     debug_progbuf_start(debug_data_start - program_buffer_bytes),
                                                                     debug_abstract_start(debug_progbuf_start - debug_abstract_size * 4),
                                                                    //  custom_base(0),
                                                                    //  FLAGS_num_harts(FLAGS_num_harts),
                                                                     hart_state(1 << field_width(max_hartid + 1)),
                                                                     hart_array_mask(max_hartid + 1)
//  rti_remaining(0)
{

  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dmi_req<>>(loc, [this](const auto &v)
                                                                              { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dmi_resp<>>(loc, [this](const auto &v)
                                                                               { return this->process(v); });

  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dm_load_cmd<>>(loc, [this](const auto &v)
                                                                                  { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dm_load_data<>>(loc, [this](const auto &v)
                                                                                   { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dm_store<>>(loc, [this](const auto &v)
                                                                               { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dmi_status<>>(loc, [this](const auto &v)
                                                                              { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::dm_model::dm_req<>>(loc, [this](const auto &v)
                                                                               { return this->process(v); });

  // Define a processor array (for the number of harts)
  for (size_t i = 0; i < max_hartid; i++)
  {
    harts[i] = std::make_unique<processor_t>(halt_on_reset);
  }

  program_buffer = std::vector<uint8_t>(program_buffer_bytes, 0);

  if (config.support_impebreak) {
    program_buffer[0] = csrsi(CSR_C_PRIV, 1);
    program_buffer[1] = csrsi(CSR_C_PRIV, 1) >> 8;
    program_buffer[2] = csrsi(CSR_C_PRIV, 1) >> 16;
    program_buffer[3] = csrsi(CSR_C_PRIV, 1) >> 24;
    program_buffer[4*config.progbufsize] = ebreak();
    program_buffer[4*config.progbufsize+1] = ebreak() >> 8;
    program_buffer[4*config.progbufsize+2] = ebreak() >> 16;
    program_buffer[4*config.progbufsize+3] = ebreak() >> 24;
  }

  memset(debug_rom_flags, 0, sizeof(debug_rom_flags));
  memset(dmdata, 0, sizeof(dmdata));

  write32(debug_rom_whereto, 0,
          jal(ZERO, debug_abstract_start - DEBUG_ROM_WHERETO));

  memset(debug_abstract, 0, sizeof(debug_abstract));

  // // Keeping everything available, but will get it from fuse map
  // for (unsigned i = 0; i < max_hartid; i++)
  // {
  //   if (i < FLAGS_num_harts) 
  //     hart_available_state[i] = true;
  //   else 
  //     hart_available_state[i] = false; 

  //   cvm::log(cvm::NONE,"hart_available_state[{:#x}] = {:#x}\n",i, hart_available_state[i]);
  // }
  
  cvm::log(cvm::NONE,"\nConstructing DM Model.. \n");
  reset();
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dmi_status<> &dmi_status)
{
  cvm::log(cvm::HIGH, "Model recieved dmi status: status {:#x} cmds in queue {:#x}\n", dmi_status.status, dmi_status.commands_in_queue);
  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
  cvm::registry::messenger.signal(tbox_loc, debugger::dmi_status_t{dmi_status.status, dmi_status.commands_in_queue});
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dmi_req<> &dmi_req)
{
  uint32_t read_value;
  cvm::log(cvm::NONE, "DMI Monitor :: dmi request with: op {:#x} addr {:#x}\n", dmi_req.op, dmi_req.addr);
  if (dmi_req.op == 1)
  {
    cvm::log(cvm::FULL, "[Misc] Poll:{:#x}\n", dmi_req.misc_signals);
    if (dmi_req.misc_signals == 0)
    {
      debug_module_t::dmi_read(dmi_req.addr, &read_value);
      cvm::log(cvm::HIGH, "The Expected DMI Read data should be: {:#x}\n", read_value);
      req_expect = read_value;

      if (dm_regs_to_check.find((uint8_t)dmi_req.addr) != dm_regs_to_check.end())
      {
        reg_addr_to_check = (uint8_t)dmi_req.addr;
        req_resp_check = true;
      }
    }
  }
  if (dmi_req.op == 2)
  {
    debug_module_t::dmi_write(dmi_req.addr, dmi_req.data);
    cvm::log(cvm::HIGH, "Model recieved dmi request: op {:#x} addr {:#x} data {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data);
  }
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dmi_resp<> &dmi_resp)
{ 
  cvm::log(cvm::NONE, "DMI Monitor :: dmi response code: {:#x} and with data: {:#x}\n", dmi_resp.resp, dmi_resp.data);
  uint32_t actual_data = dmi_resp.data;
  uint32_t masked_actual_data;
  uint32_t masked_req_expect;

  if (FLAGS_debug_enable == 0x2 || FLAGS_debug_enable == 0x0) { //TODO: Add flags when AXI traffic is enabled via DTM
    if (dmi_resp.resp != 0x2){
      if (!FLAGS_dm_model_check_bypass)
        cvm::log(cvm::ERROR, "[Error] Expected an Error response as debug is disabled via DTM, but got a response of {:#x}}\n", dmi_resp.resp);
      else
        cvm::log(cvm::NONE, "[Mismatch] Expected an Error response as debug is disabled via DTM, but got a response of {:#x}}\n", dmi_resp.resp); 
    }
    return;
  }  
  
  if (req_resp_check)
  {
    if ((actual_data != req_expect) && !dmstatus.ndmresetpending){
      if(!FLAGS_dm_model_check_bypass)
        cvm::log(cvm::ERROR, "[Error] Seen a DMI Response Mismatch for Addr:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, actual_data, req_expect);
      else
        cvm::log(cvm::NONE, "[Mismatch] Seen a DMI Response Mismatch for Addr:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, actual_data, req_expect); 
    }
    else if ((actual_data != req_expect) && dmstatus.ndmresetpending)
    {
      cvm::log(cvm::HIGH, " Values seen for Addr:{:#x} before masking ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, actual_data, req_expect);
      masked_actual_data = actual_data & 0xFFFFF0FF;
      masked_req_expect = req_expect & 0xFFFFF0FF;
      cvm::log(cvm::HIGH, "Ndmresetpending is 1 masking dmstatus[11:8]\n");

        if ((masked_actual_data != masked_req_expect)){
          if(!FLAGS_dm_model_check_bypass)
            cvm::log(cvm::ERROR, "[Error] Seen a DMI Response Mismatch for Addr:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, masked_actual_data, masked_req_expect);
          else
            cvm::log(cvm::NONE, "[Mismatch] Seen a DMI Response Mismatch for Addr:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, masked_actual_data, masked_req_expect); 
        }
        else
          cvm::log(cvm::HIGH, "Masked_actual_data :{:#x} and Masked_req_expected :{:#x} are matching\n", masked_actual_data, masked_req_expect);
    }
    req_resp_check = false;
    return;
  }
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_load_cmd<> &dm_load_cmd)
{
  load_req_length = dm_load_cmd.size;

  cvm::log(cvm::NONE, "DMI Monitor:: load request sent for the address:{:#x} for length:{:#x} with id:{:#x}\n", dm_load_cmd.addr, load_req_length, dm_load_cmd.id);

  cvm::log(cvm::HIGH, "[{}] Sent a load req for the address:{:#x} for length:{:#x} with id:{:#x}\n", sent_count, dm_load_cmd.addr, load_req_length, dm_load_cmd.id);
  sent_count++;
  
  debug_module_t::load(dm_load_cmd.addr, load_req_length, load_bytes_data);

  std::memcpy(&expected_load_data, load_bytes_data, load_req_length);
  cvm::log(cvm::HIGH, "The Load data expected for id:{:#x}, is data:{:#x}\n", dm_load_cmd.id, expected_load_data);

  load_req_id = dm_load_cmd.id;
  load_req_addr = dm_load_cmd.addr;
  mem_load_check = !((dm_load_cmd.addr >= 0x2c0) && (dm_load_cmd.addr <= 0x2f0)); // Don't do a load check on the reserved regions
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_load_data<> &dm_load_data)
{
  cvm::log(cvm::NONE, "DMI Monitor:: load resp for the id:{:#x} and data:{:#x}\n", dm_load_data.id, dm_load_data.data);

  cvm::log(cvm::HIGH, "[{}] Got a load resp for the id:{:#x} and data:{:#x}\n", resp_count, dm_load_data.id, dm_load_data.data);
  resp_count++;

  if (load_req_id == dm_load_data.id && mem_load_check)
  {
    cvm::log(cvm::HIGH, "Seen a matching load response for the same id as the previous load request\n");
    uint64_t expected_load_data_to_check = cvm::bitmanip::slice<uint64_t>(expected_load_data, (load_req_length * 4 - 1), 0);
    uint64_t actual_load_data_to_check = cvm::bitmanip::slice<uint64_t>(dm_load_data.data, (load_req_length * 4 - 1), 0);
    uint64_t expected_debug_load_data_to_check = cvm::bitmanip::slice<uint64_t>(expected_load_data, (load_req_length * 8 - 1), 0);
    uint64_t actual_debug_load_data_to_check = cvm::bitmanip::slice<uint64_t>(dm_load_data.data, (load_req_length * 8 - 1), 0);

    if ((expected_load_data_to_check != actual_load_data_to_check) && !(load_req_addr >= DEBUG_ROM_FLAGS) && !(load_req_addr < (DEBUG_ROM_FLAGS + 8))) {
      if(load_req_addr == DEBUG_ROM_FLAGS && !reflow_flags) {
        reflow_flags = true;
        cvm::log(cvm::HIGH, "Reflowing 0x400 read to allow DM to update state flag\n");
      }
      // else if ((load_req_addr >= DEBUG_ROM_FLAGS) && (load_req_addr < (DEBUG_ROM_FLAGS + 8)) && ((load_req_addr & 0x0000000f) != hart_haltreq_hg))
      // {
      //   cvm::log(cvm::HIGH, "[Test] expected_load_data_to_check:{:#x} hart_select:{:#x} \n", expected_load_data_to_check, hart_select);
      //   cvm::log(cvm::HIGH, "[Test] actual_load_data_to_check:{:#x} hart_select:{:#x} \n", actual_load_data_to_check, hart_select);
      //   shifted_actual_data = (actual_load_data_to_check >> (8 * hart_select));
      //   cvm::log(cvm::HIGH, "[Test] shifted_actual_data:{:#x} hart_select:{:#x} \n", shifted_actual_data, hart_select);
      //   //shifted_expected_data = (expected_load_data_to_check >> (8 * hart_select));
      //   masked_actual_data = shifted_actual_data & 0x0000000F;
      //   masked_expected_data = expected_load_data_to_check & 0x0000000F;
      //   cvm::log(cvm::HIGH, "[Test] masked_actual_data:{:#x} hart_select:{:#x} \n", masked_actual_data, hart_select);
      //   cvm::log(cvm::HIGH, "[Test] masked_expected_data:{:#x} hart_select:{:#x} \n", masked_expected_data, hart_select);
      //   if ((masked_actual_data != masked_expected_data))
      //   {
      //   cvm::log(!FLAGS_dm_model_check_bypass?cvm::ERROR:cvm::NONE, "[Mismatch] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Shifted_Masked_Actual:{:#x} vs Masked_Expected:{:#x}\n", load_req_addr, load_req_length, masked_actual_data, masked_expected_data);
      //   } 
      // }
      else {
        reflow_flags = false;
        if (!FLAGS_dm_model_check_bypass)
          cvm::log(cvm::ERROR, "[Error] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n",load_req_addr,load_req_length,actual_load_data_to_check,expected_load_data_to_check);
        else 
          cvm::log(cvm::NONE, "[Mismatch] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n",load_req_addr,load_req_length,actual_load_data_to_check,expected_load_data_to_check);
      }
    }
    else if ((expected_debug_load_data_to_check != actual_debug_load_data_to_check) && (load_req_addr >= DEBUG_ROM_FLAGS) && (load_req_addr < (DEBUG_ROM_FLAGS + 8)) && ((load_req_addr & 0x0000000f) != hart_haltreq_hg) && hart_state[0x0000000F & load_req_addr].resumegroup)
      {
        uint64_t shifted_actual_data, hart_select, masked_actual_data, masked_expected_data;
      hart_select = 0x0000000F & load_req_addr;
      cvm::log(cvm::HIGH, "[Test] hart_haltreq_hg:{:#x}  \n", hart_haltreq_hg);
      cvm::log(cvm::HIGH, "[Test] Resumegroup:{:#x} hart_select:{:#x} \n",hart_state[hart_select].resumegroup, hart_select);
      cvm::log(cvm::HIGH, "[Test] expected_debug_load_data_to_check:{:#x} hart_select:{:#x} \n", expected_debug_load_data_to_check, hart_select);
      cvm::log(cvm::HIGH, "[Test] actual_debug_load_data_to_check:{:#x} hart_select:{:#x} \n", actual_debug_load_data_to_check, hart_select);
      shifted_actual_data = (actual_debug_load_data_to_check >> (8 * hart_select));
      cvm::log(cvm::HIGH, "[Test] shifted_actual_data:{:#x} hart_select:{:#x} \n", shifted_actual_data, hart_select);
      //shifted_expected_data = (expected_load_data_to_check >> (8 * hart_select));
        masked_actual_data = shifted_actual_data & 0x0000000F;
        masked_expected_data = expected_debug_load_data_to_check & 0x0000000F;
        cvm::log(cvm::HIGH, "[Test] masked_actual_data:{:#x} hart_select:{:#x} \n", masked_actual_data, hart_select);
        cvm::log(cvm::HIGH, "[Test] masked_expected_data:{:#x} hart_select:{:#x} \n", masked_expected_data, hart_select);
        if ((masked_actual_data != masked_expected_data) && (masked_actual_data != 0x2)) {
          if (!FLAGS_dm_model_check_bypass) 
            cvm::log(cvm::HIGH, "[Error] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Shifted_Masked_Actual:{:#x} vs Masked_Expected:{:#x}\n", load_req_addr, load_req_length, masked_actual_data, masked_expected_data);
          else
            cvm::log(cvm::NONE, "[Mismatch] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Shifted_Masked_Actual:{:#x} vs Masked_Expected:{:#x}\n", load_req_addr, load_req_length, masked_actual_data, masked_expected_data);
        }
    }
  }
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_store<> &dm_store)
{
  cvm::log(cvm::NONE, "DMI Monitor :: Store req for the address:{:#x} and len:{:#x} and data:{:#x}\n", dm_store.addr, dm_store.len, dm_store.data);

  uint8_t *store_data = (uint8_t *)&dm_store.data;
  debug_module_t::store(dm_store.addr, 4, store_data);
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_req<> &dm_req)
{

  std::istringstream ss(FLAGS_hart_enable_id);
  std::string token;
  while (std::getline(ss, token, ',')) {
    if (token != "") {
      uint32_t t = std::stoull(token);
      hart_pid.push_back(t);
      hart_pid_mask |= (1 << t);
    }
  }
  num_pid_harts = std::bitset<32>(hart_pid_mask).count();
  cvm::log(cvm::NONE, "DMI Monitor :: DM req for the Physical Hart ID :{:#x} dmcontrol.hartsel: {} num_pid_harts : {}\n", dm_req.dm_ms_req, dmcontrol.hartsel,num_pid_harts);
  if(FLAGS_dm_hart_enum_chk){
  for (size_t i = 0; i < hart_pid.size(); i++) {
        cvm::log(cvm::HIGH, "DMI Monitor :: Hart VID: {}  PID : {} \n",i, hart_pid[i]);
  }
  // Check if access went to correct PID
if (dm_req.dm_ms_req) {
  // Validate that the PID for the selected VID is included in the dm_ms_req mask
  uint32_t expected_pid = hart_pid[dmcontrol.hartsel];
  if (dm_req.dm_ms_req & (1U << expected_pid)) {
    cvm::log(cvm::MEDIUM, "DM VID/PID CHECKER :: Correct DM req includes Physical Hart ID :{:#x} dmcontrol.hartsel: {} hart_pid[dmcontrol.hartsel]: {}\n",
             dm_req.dm_ms_req, dmcontrol.hartsel, expected_pid);
  } else {
    cvm::log(cvm::ERROR, "ERROR: DM VID/PID CHECKER :: Incorrect DM req does not include Physical Hart ID :{:#x} dmcontrol.hartsel: {} hart_pid[dmcontrol.hartsel]: {}\n",
             dm_req.dm_ms_req, dmcontrol.hartsel, expected_pid);
  }
}
  }
}
void debug_module_t::init_debug_abstract_buffer(){
  unsigned i = 0;
  debug_module_t::write32(debug_abstract,i++, ZERO);    // 0 (low)
  (has_second_scratch)? debug_module_t::write32(debug_abstract,i++,auipc(A0,ZERO)) : debug_module_t::write32(debug_abstract, i++, nop());    // 0 (high)
  (has_second_scratch)? debug_module_t::write32(debug_abstract,i++,srli(A0,A0,12)) : debug_module_t::write32(debug_abstract, i++, nop());    // 1 (low)
  (has_second_scratch)? debug_module_t::write32(debug_abstract,i++,slli(A0,A0,12)) : debug_module_t::write32(debug_abstract, i++, nop());    // 1 (high)
  debug_module_t::write32(debug_abstract, i++, nop());    // 2 (low)
  debug_module_t::write32(debug_abstract, i++, nop());    // 2 (high)
  debug_module_t::write32(debug_abstract, i++, nop());    // 3 (low)
  debug_module_t::write32(debug_abstract, i++, nop());    // 3 (high)
  (has_second_scratch)? debug_module_t::write32(debug_abstract,i++,csrr(A0,CSR_DSCRATCH1)) : debug_module_t::write32(debug_abstract, i++, nop());    // 4 (low)
  debug_module_t::write32(debug_abstract, i++, ebreak()); // 4 (high)
}

void debug_module_t::reset()
{
  cvm::log(cvm::NONE,"\nReset DM Model.. \n");
  cvm::log(cvm::HIGH, "[Reset Harts]\n"); //Fixed value as per the implementation

  // Keeping everything available, but will get it from fuse map
  for (unsigned i = 0; i < max_hartid; i++)
  {
    if (i < FLAGS_num_harts) 
      hart_available_state[i] = true;
    else 
      hart_available_state[i] = false; 

    cvm::log(cvm::HIGH,"hart_available_state[{:#x}] = {:#x}\n",i, hart_available_state[i]);
  }
  
  for (const auto &[hart_id, hart] : harts)
  { // harts
    hart->halt_request = hart->HR_NONE;
    hart_state[hart_id].resumeack = false;
  }

  memset(&dmcontrol, 0, sizeof(dmcontrol));
  memset(&dmcs2, 0, sizeof(dmcs2));
  memset(&dmstatus, 0, sizeof(dmstatus));
  dmstatus.impebreak = config.support_impebreak;
  dmstatus.authenticated = !config.require_authentication;
  dmstatus.version = 3;

  memset(&abstractcs, 0, sizeof(abstractcs));
  abstractcs.datacount = sizeof(dmdata) / 4;
  abstractcs.progbufsize = config.progbufsize;

  memset(&abstractauto, 0, sizeof(abstractauto));

  for (auto &ele : hart_state)
  { // Add havereset for all the core
    ele.havereset = true;
  }

  cvm::log(cvm::HIGH, "[Config] debug_data_start={:#x}\n", 0x388); //Fixed value as per the implementation
  cvm::log(cvm::HIGH, "[Config] debug_progbuf_start={:#x}\n", debug_progbuf_start);
  cvm::log(cvm::HIGH, "[Config] debug_abstract_start={:#x}\n", debug_abstract_start);

  // Initialize the Debug Abstract Command Buffer
  init_debug_abstract_buffer();
}

bool debug_module_t::load(reg_t addr, size_t len, uint8_t *bytes)
{
  if (len == 0)
    len = 1;
  addr = DEBUG_START + addr;
  cvm::log(cvm::HIGH, "Load Request Addr:{:#x}, len:{:#x} first_range:{:#x}\n", addr, len, (DEBUG_ROM_ENTRY + debug_rom_raw_len));

  if (addr >= DEBUG_ROM_ENTRY &&
      (addr + len) <= (DEBUG_ROM_ENTRY + debug_rom_raw_len)) {
    cvm::log(cvm::FULL, "Debug ROM Case::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_rom_raw + addr - DEBUG_ROM_ENTRY, len);
    return true;
  }

  if ((addr + len) >= (DEBUG_ROM_ENTRY + debug_rom_raw_len) && (addr + len) < (DEBUG_ROM_ENTRY + debug_rom_raw_len + debug_rom_raw_upper_rsvd_len)) {
    cvm::log(cvm::FULL, "Upper half of Debug ROM ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_rom_raw_upper_rsvd + addr - DEBUG_ROM_ENTRY - debug_rom_raw_len, len);
    return true; 
  }

  if (addr >= DEBUG_ROM_WHERETO && (addr + len) <= (DEBUG_ROM_WHERETO + 8)) // FIXME: Check if this should be 4 or 8
  {
    cvm::log(cvm::FULL, "Whereto Case ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_rom_whereto + addr - DEBUG_ROM_WHERETO, len);
    return true;
  }

  if (addr >= DEBUG_ROM_FLAGS && ((addr + len) <= DEBUG_ROM_FLAGS + 1024))
  {
    cvm::log(cvm::FULL, "Debug ROM Flags ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_rom_flags + addr - DEBUG_ROM_FLAGS, len);
    return true;
  }

  if (addr >= debug_abstract_start && ((addr + len) <= (debug_abstract_start + sizeof(debug_abstract))))
  {
    cvm::log(cvm::FULL, "Abstract Command Buffer ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_abstract + addr - debug_abstract_start, len);
    return true;
  }

  if (addr >= debug_data_start && (addr + len) <= (debug_data_start + sizeof(dmdata)))
  {
    cvm::log(cvm::FULL, "Debug Data Region ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, dmdata + addr - debug_data_start, len);
    return true;
  }

  if (addr >= debug_progbuf_start && ((addr + len) <= (debug_progbuf_start + program_buffer_bytes)))
  {
    cvm::log(cvm::FULL, "Program Buffer Region ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, program_buffer.data() + addr - debug_progbuf_start, len);
    return true;
  }

  if ((addr >= 0x2c0 && addr <= 0x2f0) || ( addr >= 0x3b8 && addr <0x400)) 
  {
    cvm::log(cvm::FULL, "Reserved space ::: Addr={:#x}, Length={:#x}\n",addr,len);
    return true;
  }

  if (!FLAGS_dm_model_check_bypass)
    cvm::log(cvm::ERROR, "ERROR: invalid load from debug module --> len:{:#x} at addr:{:#x}\n", len, addr);
  else
    cvm::log(cvm::NONE, "Mismatch: invalid load from debug module --> len:{:#x} at addr:{:#x}\n", len, addr);

  return false;
}

bool debug_module_t::store(reg_t addr, size_t len, const uint8_t *bytes)
{

  switch (len)
  {
  case 4:
    cvm::log(cvm::HIGH, "store(addr={:#x}, len={:#x}, bytes={:#x}); "
                        "hartsel={:#x}\n",
             addr, (unsigned)len, *(uint32_t *)bytes,
             dmcontrol.hartsel);
    break;
  default:
    cvm::log(cvm::HIGH, "store(addr={:#x}, len={:#x}, bytes=...); "
                        "hartsel={:#x}\n",
             addr, (unsigned)len, dmcontrol.hartsel);
    break;
  };

  uint8_t id_bytes[4];
  uint32_t id = 0;
  if (len == 4)
  {
    memcpy(id_bytes, bytes, 4);
    id = read32(id_bytes, 0);
  }

  addr = DEBUG_START + addr;

  if (addr >= debug_data_start && (addr + len) <= (debug_data_start + sizeof(dmdata)))
  {
    memcpy(dmdata + addr - debug_data_start, bytes, len);
    return true;
  }

  if (addr >= debug_progbuf_start && ((addr + len) <= (debug_progbuf_start + program_buffer_bytes)))
  {
    memcpy(program_buffer.data() + addr - debug_progbuf_start, bytes, len);

    return true;
  }

  if (addr == DEBUG_ROM_HALTED)
  {
    // if((hart_haltreq_hg != dmcontrol.hartsel) && hartsel_stable) 
    // {
    //   cvm::log(cvm::HIGH,"if hartsel_stable:{:#x}, hart_haltreq_hg:{:#x}, dmcontrol.hartsel:{:#x} \n",hartsel_stable, hart_haltreq_hg, dmcontrol.hartsel);
    //   hartsel_stable = false;
    // }
    // else 
    // {
    //   cvm::log(cvm::HIGH,"else hartsel_stable:{:#x}, hart_haltreq_hg:{:#x}, dmcontrol.hartsel:{:#x} \n",hartsel_stable, hart_haltreq_hg, dmcontrol.hartsel);
    // }
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Halted State\n");
    assert(len == 4);
    //if (!hart_state[id].halted && (hart_haltreq_hg != dmcontrol.hartsel))
    if (!hart_state[id].halted)
    {
      cvm::log(cvm::HIGH, " hart_state[id = :{:#x}] is not halted\n", id);
      hart_state[id].halted = true;
      if (hart_state[id].haltgroup)
      {
        cvm::log(cvm::HIGH, "hart_state[id = :{:#x}] is part of haltgroup\n", id);
        if(hartsel_stable == false) {
          cvm::log(cvm::HIGH, " before debug_rom_flags haltgrp clear :{:#x}, :{:#x}\n", id, debug_rom_flags[id]);
          debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_HALTGRP);
          cvm::log(cvm::HIGH, "debug_rom_flags haltgrp clear :{:#x}, :{:#x}\n", id, debug_rom_flags[id]);
        }
      }
      else { 
        cvm::log(cvm::HIGH, "hart_state[id = :{:#x}] is not part of haltgroup, :{:#x}\n", id, hart_state[id].haltgroup);
      }
    }

    if (selected_hart_id() == id)
    {
      //cvm::log(cvm::HIGH, "selected_hart_id() :{:#x}] id:{:#x}] \n",selected_hart_id(), id);
      if((hart_haltreq_hg == id || hart_abscmd == id) && (hart_haltreq_hg != 0) && (hart_abscmd != 0))
      {
        debug_rom_flags[id] = 0;
        cvm::log(cvm::HIGH, "[Display] debug_rom_flags[id] :{:#x}] id:{:#x}] \n", debug_rom_flags[id], id);
        cvm::log(cvm::HIGH, "[Display] hart_haltreq_hg :{:#x}] hart_abscmd :{:#x}] \n", hart_haltreq_hg, hart_abscmd);
      }
      else {
        cvm::log(cvm::HIGH, "[Display] Executing else in line 440\n");
      }
      if (0 == (debug_rom_flags[id] & (1 << DEBUG_ROM_FLAG_GO)))
      {
        // abstract_command_completed = true;
        cvm::log(cvm::HIGH, " busy is cleared in abstractcs.busy:{:#x}, debug_rom_flags[id]:{:#x} dmcontrol.hartsel:{:#x} id :{:#x}\n", abstractcs.busy, debug_rom_flags[id], dmcontrol.hartsel, id);
        abstractcs.busy = false;
      }
      else {
        cvm::log(cvm::HIGH, " busy is not cleared in abstractcs.busy:{:#x}, debug_rom_flags[id]:{:#x} dmcontrol.hartsel:{:#x} id :{:#x} \n", abstractcs.busy, debug_rom_flags[id], dmcontrol.hartsel, id);
      }
    }
    return true;
  }

  if (addr == DEBUG_ROM_GOING)
  {
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Going State\n");
    assert(len == 4);
    //debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_GO); //correct
    cvm::log(cvm::HIGH, "[Display] Before clearing debug_rom_flags[hart_abscmd]:{:#x}, hart_abscmd:{:#x} \n", debug_rom_flags[hart_abscmd], hart_abscmd );
    debug_rom_flags[hart_abscmd] = 0;
    cvm::log(cvm::HIGH, "[Display] After clearing debug_rom_flags[hart_abscmd]:{:#x}, hart_abscmd:{:#x} \n", debug_rom_flags[hart_abscmd], hart_abscmd );
    return true;
  }

  if (addr == DEBUG_ROM_RESUMING)
  {
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Resume State\n");
    assert(len == 4);
    hart_state[id].halted = false;
    hart_state[id].resumeack = true;
    cvm::log(cvm::HIGH, "before debug_rom_flags resume  clear :{:#x}, :{:#x}\n", id, debug_rom_flags[id]);
    debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_RESUME);
    cvm::log(cvm::HIGH, "debug_rom_flags resume  clear :{:#x}, :{:#x}\n", id, debug_rom_flags[id]);
    return true;
  }

  if (addr == DEBUG_ROM_EXCEPTION)
  {
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Exception State\n");
    if (abstractcs.cmderr == CMDERR_NONE)
    {
      cvm::log(cvm::HIGH, "Cmderr was set to Exception\n");
      abstractcs.cmderr = CMDERR_EXCEPTION;
    }
    return true;
  }

  if (!FLAGS_dm_model_check_bypass)
    cvm::log(cvm::ERROR, "ERROR: invalid store to debug module --> len:{:#x} at addr:{:#x}\n", len, addr);
  else
    cvm::log(cvm::NONE, "Mismatch: invalid store to debug module --> len:{:#x} at addr:{:#x}\n", len, addr);

  return false;
}

void debug_module_t::write32(uint8_t *memory, unsigned int index, uint32_t value)
{
  uint8_t *base = memory + index * 4;
  base[0] = value & 0xff;
  base[1] = (value >> 8) & 0xff;
  base[2] = (value >> 16) & 0xff;
  base[3] = (value >> 24) & 0xff;
}

uint32_t debug_module_t::read32(uint8_t *memory, unsigned int index)
{
  uint8_t *base = memory + index * 4;
  uint32_t value = ((uint32_t)base[0]) |
                   (((uint32_t)base[1]) << 8) |
                   (((uint32_t)base[2]) << 16) |
                   (((uint32_t)base[3]) << 24);
  return value;
}

bool debug_module_t::hart_selected(unsigned hartid) const
{
  return hartid == selected_hart_id() && (hartid < max_hartid); // || (dmcontrol.hasel && hart_array_mask[hartid]);
  cvm::log(cvm::HIGH, "Hart_selected func, hartid = {:#x}", hartid);
}

bool debug_module_t::hart_available(unsigned hart_id) const
{
  if (hart_id < FLAGS_num_harts) { //FIXME
    cvm::log(cvm::HIGH, "Hart_available comparison func, hart_id = {:#x}, NUM_HARTS = {:#x}, state = {:#x}\n", hart_id, FLAGS_num_harts, hart_available_state[hart_id]);
    return hart_available_state[hart_id];
  }
  else 
    return false;
}

bool debug_module_t::dmi_read(unsigned address, uint32_t *value)
{
  uint32_t result = 0;
  cvm::log(cvm::HIGH, "dmi_read to addr:{:#x}\n", address);
  if (address >= DM_DATA0 && address < DM_DATA0 + abstractcs.datacount)
  {
    unsigned i = address - DM_DATA0;
    result = read32(dmdata, i);
    if (abstractcs.busy)
    {
      result = -1;
      cvm::log(cvm::HIGH, "dmi_read({:#x} (data[{}]) -> -1 because abstractcs.busy==true\n", address, i);
    }

    if (abstractcs.busy && abstractcs.cmderr == CMDERR_NONE)
    {
      cvm::log(cvm::HIGH,"Setting cmderr to Busy in the DMI_Read func\n");
      abstractcs.cmderr = CMDERR_BUSY;
    }

    if (!abstractcs.busy && ((abstractauto.autoexecdata >> i) & 1))
    {
      perform_abstract_command();
    }
  }
  else if (address >= DM_PROGBUF0 && address < DM_PROGBUF0 + config.progbufsize)
  {
    unsigned i = address - DM_PROGBUF0;
    result = read32(program_buffer.data(), config.support_impebreak?(i+1):i);
    if (abstractcs.busy)
    {
      result = -1;
      cvm::log(cvm::HIGH, "dmi_read({:#x} (progbuf[{}]) -> -1 because abstractcs.busy==true\n", address, i);
    }
    if (!abstractcs.busy && ((abstractauto.autoexecprogbuf >> i) & 1))
    {
      perform_abstract_command();
    }
  }
  else
  { 
    switch (address)
    {
    case DM_DMCONTROL:
    {
      result = set_field(result, DM_DMCONTROL_HALTREQ, dmcontrol.haltreq);
      result = set_field(result, DM_DMCONTROL_RESUMEREQ, dmcontrol.resumereq);
      result = set_field(result, DM_DMCONTROL_ACKHAVERESET, dmcontrol.ackhavereset);
      result = set_field(result, DM_DMCONTROL_HARTSELHI,
                         dmcontrol.hartsel >> DM_DMCONTROL_HARTSELLO_LENGTH);
      result = set_field(result, DM_DMCONTROL_HASEL, dmcontrol.hasel);
      result = set_field(result, DM_DMCONTROL_HARTSELLO, dmcontrol.hartsel);
      result = set_field(result, DM_DMCONTROL_HARTRESET, dmcontrol.hartreset);
      result = set_field(result, DM_DMCONTROL_NDMRESET, dmcontrol.ndmreset);
      result = set_field(result, DM_DMCONTROL_DMACTIVE, dmcontrol.dmactive);
    }
    break;
    case DM_DMSTATUS:
    {
      dmstatus.allhalted = true;
      dmstatus.anyhalted = false;
      dmstatus.allrunning = true;
      dmstatus.anyrunning = false;
      dmstatus.allnonexistant = true;
      dmstatus.allresumeack = true;
      dmstatus.anyresumeack = false;
      dmstatus.allunavail = true;
      dmstatus.anyunavail = false;
      for (const auto &[hart_id, hart] : harts)
      {
        if (hart_selected(hart_id))
        {
          cvm::log(cvm::HIGH, "Inside the dmstatus read, Loop for hart_id = {:#x}\n", hart_id);
          dmstatus.allnonexistant = false;
          if (hart_state[hart_id].resumeack)
          {
            cvm::log(cvm::FULL, "Inside the resumeack state (dmstatus read), for hart_id = {:#x}\n", hart_id); 
            dmstatus.anyresumeack = true;
          }
          else
          {
            dmstatus.allresumeack = false;
          }
          // auto hart = harts.at(hart_id);
          if (hart_state[hart_id].halted)
          {
            cvm::log(cvm::FULL, "Inside the halted state (dmstatus read), for hart_id = {:#x}\n", hart_id); 
            dmstatus.allrunning = false;
            dmstatus.anyhalted = true;
            dmstatus.allunavail = false;
          }
          else if (!hart_available(hart_id))
          {
            cvm::log(cvm::FULL, "Inside the not available state (dmstatus read), for hart_id = {:#x}\n", hart_id); 
            dmstatus.allrunning = false;
            dmstatus.allhalted = false;
            dmstatus.anyunavail = true;
          }
          else
          {
            dmstatus.allhalted = false;
            dmstatus.anyrunning = true;
            dmstatus.allunavail = false;
          }
        }
      }

      // We don't allow selecting non-existant harts through
      // hart_array_mask, so the only way it's possible is by writing a
      // non-existant hartsel.
      dmstatus.anynonexistant = dmcontrol.hartsel >= max_hartid;

      result = set_field(result, DM_DMSTATUS_NDMRESETPENDING, dmstatus.ndmresetpending);
      result = set_field(result, DM_DMSTATUS_IMPEBREAK,
                         dmstatus.impebreak);
      result = set_field(result, DM_DMSTATUS_ALLHAVERESET, selected_hart_state().havereset);
      result = set_field(result, DM_DMSTATUS_ANYHAVERESET, selected_hart_state().havereset);
      result = set_field(result, DM_DMSTATUS_ALLNONEXISTENT, dmstatus.allnonexistant);
      result = set_field(result, DM_DMSTATUS_ALLUNAVAIL, dmstatus.allunavail);
      result = set_field(result, DM_DMSTATUS_ALLRUNNING, dmstatus.allrunning);
      result = set_field(result, DM_DMSTATUS_ALLHALTED, dmstatus.allhalted);
      result = set_field(result, DM_DMSTATUS_ALLRESUMEACK, dmstatus.allresumeack);
      result = set_field(result, DM_DMSTATUS_ANYNONEXISTENT, dmstatus.anynonexistant);
      result = set_field(result, DM_DMSTATUS_ANYUNAVAIL, dmstatus.anyunavail);
      result = set_field(result, DM_DMSTATUS_ANYRUNNING, dmstatus.anyrunning);
      result = set_field(result, DM_DMSTATUS_ANYHALTED, dmstatus.anyhalted);
      result = set_field(result, DM_DMSTATUS_ANYRESUMEACK, dmstatus.anyresumeack);
      result = set_field(result, DM_DMSTATUS_AUTHENTICATED, dmstatus.authenticated);
      result = set_field(result, DM_DMSTATUS_AUTHBUSY, dmstatus.authbusy);
      result = set_field(result, DM_DMSTATUS_VERSION, dmstatus.version);
    }
    break;
    case DM_ABSTRACTCS:
      result = set_field(result, DM_ABSTRACTCS_CMDERR, abstractcs.cmderr);
      result = set_field(result, DM_ABSTRACTCS_BUSY, abstractcs.busy);
      result = set_field(result, DM_ABSTRACTCS_DATACOUNT, abstractcs.datacount);
      result = set_field(result, DM_ABSTRACTCS_PROGBUFSIZE,
                         abstractcs.progbufsize);
      break;
    case DM_ABSTRACTAUTO:
      result = set_field(result, DM_ABSTRACTAUTO_AUTOEXECPROGBUF, abstractauto.autoexecprogbuf);
      result = set_field(result, DM_ABSTRACTAUTO_AUTOEXECDATA, abstractauto.autoexecdata);
      break;
    case DM_COMMAND:
      result = 0;
      break;
    case DM_HARTINFO:
      result = set_field(result, DM_HARTINFO_NSCRATCH, 1);
      result = set_field(result, DM_HARTINFO_DATAACCESS, 1);
      result = set_field(result, DM_HARTINFO_DATASIZE, abstractcs.datacount);
      result = set_field(result, DM_HARTINFO_DATAADDR, debug_data_start);
      break;
    case DM_AUTHDATA:
      result = 0; // challenge;
      break;
    case DM_HAWINDOWSEL:
      result = hawindowsel;
      break;
    case DM_HAWINDOW:
      {
        unsigned base = hawindowsel * 32;
        for (unsigned i = 0; i < 32; i++) {
          unsigned n = base + i;
          if (n < max_hartid && hart_array_mask[n]) { //FIXME: Change it later :: hart_array_mask[sim->get_cfg().hartids()[n]]
            result |= 1 << i;
          }
        }
      }
      break;
    case DM_DMCS2:
      dmcs2.hgwrite = 0;
      result = set_field(result, DM_DMCS2_GROUPTYPE, dmcs2.grouptype);
      result = set_field(result, DM_DMCS2_DMEXTTRIGGER, dmcs2.dmexttrigger);
      result = set_field(result, DM_DMCS2_GROUP, dmcs2.grouptype?selected_hart_state().resumegroup:selected_hart_state().haltgroup);
      result = set_field(result, DM_DMCS2_HGWRITE, dmcs2.hgwrite);
      result = set_field(result, DM_DMCS2_HGSELECT, dmcs2.hgselect);
      break;
    case DM_NEXTDM:
      result = 0;
      break;
    case DM_HALTSUM0:
      {
        result = 0;
        for (unsigned id = 0; id < FLAGS_num_harts; id++) {
          if (hart_state[id].halted) {
            result |= 1 << id;
          }
        }
      }
      break;
    default:
      result = 0;

      if (!FLAGS_dm_model_check_bypass & !FLAGS_dry_space_access)
        cvm::log(cvm::ERROR, "Unexpected Register [Maybe not part of this implementation]. Returning Error.\n");
      else 
        cvm::log(cvm::NONE, "Unexpected Register [Maybe not part of this implementation] \n");

      if (FLAGS_dry_space_access)
        return true;
      else
        return false;

    }
  }
  cvm::log(cvm::HIGH, "{:#x}", result);
  *value = result;
  return true;
}

// void debug_module_t::run_test_idle()
// {
//   if (rti_remaining > 0)
//   {
//     rti_remaining--;
//   }
//   if (rti_remaining == 0 && abstractcs.busy && abstract_command_completed)
//   {
//     abstractcs.busy = false;
//   }
// }

bool debug_module_t::perform_abstract_command()
{
  init_debug_abstract_buffer();

  // abstractcs.cmderr = CMDERR_NONE; 

  cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an abstract command\n");
  hart_abscmd = dmcontrol.hartsel;
  cvm::log(cvm::HIGH, "[Display] hart_abscmd :{:#x}] \n", hart_abscmd);

  if (abstractcs.busy)
  {
    cvm::log(cvm::HIGH,"Setting cmderr to Busy in the perform abstract func\n");
    abstractcs.cmderr = CMDERR_BUSY;
    return true;
  }

  if ((command >> 24) == 0)
  {
    // register access
    unsigned size = get_field(command, AC_ACCESS_REGISTER_AARSIZE);
    bool write = get_field(command, AC_ACCESS_REGISTER_WRITE);
    unsigned regno = get_field(command, AC_ACCESS_REGISTER_REGNO);
    bool transfer = get_field(command, AC_ACCESS_REGISTER_TRANSFER);
    bool postexec = get_field(command,AC_ACCESS_REGISTER_POSTEXEC);
    bool unsupported_command = false;
    bool csr_access = false;
    bool fpr_access = false;
    if (size > 3){
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      unsupported_command = true;
      return true;
    }

    if (postexec==0 && transfer==0) // Implementation does not support this config and marks it as unsupported
    {
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      unsupported_command = true;
      return true;
    }
    if (get_field(command, AC_ACCESS_REGISTER_AARPOSTINCREMENT)){
      //write32(debug_abstract, 0, ebreak());
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      unsupported_command = true;
      return true;
    }

    if (cvm::bitmanip::slice<uint64_t>(regno, 15, 14) != 0){
      //write32(debug_abstract, 0, ebreak()); // Command not supported
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      unsupported_command = true;
      return true;
    }

    //** Custom logic to meet the implementation specific details
    if(transfer){
      csr_access = (cvm::bitmanip::slice<uint64_t>(regno, 12, 12) != 1);
      fpr_access = cvm::bitmanip::slice<uint64_t>(regno, 5, 5);
      (has_second_scratch)? write32(debug_abstract, 0, csrw(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 0, nop()); // store s0 in dscratch
      cvm::log(cvm::HIGH, "[Abstract Cmd] csr_access = :{:#x} fpr_access = :{:#x}\n",csr_access,fpr_access);
    }
    if (write)//Write to Register
    { 
      if (csr_access == false) // If GPR/FPR access
      {
        if (fpr_access) // determine whether we want to access the floating point register or not
        {
          unsigned fprnum = regno - 0x1020;
          switch(size){
           case 0:
            write32(debug_abstract, 4, flh(fprnum, load_base_address, debug_data_start));break;
           case 1:
            write32(debug_abstract, 4, flh(fprnum, load_base_address, debug_data_start));break;
           case 2:
            write32(debug_abstract, 4, flw(fprnum, load_base_address, debug_data_start));break;
           case 3:
            write32(debug_abstract, 4, fld(fprnum, load_base_address, debug_data_start));break;
          }
        }
        else
        {
          unsigned regnum = regno - 0x1000;

          if (regnum == 10){//Overwrite the Dscratch1 csr instead
            write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
            switch (size) {//Load Arg0 into S0
              case 0: 
                write32(debug_abstract, 5, lb(S0, load_base_address, debug_data_start ));break;
              case 1: 
                write32(debug_abstract, 5, lh(S0, load_base_address, debug_data_start ));break;
              case 2: 
                write32(debug_abstract, 5, lw(S0, load_base_address, debug_data_start ));break;
              case 3: 
                write32(debug_abstract, 5, ld(S0, load_base_address, debug_data_start ));break;
              }        
            write32(debug_abstract, 6, csrw(S0, CSR_DSCRATCH1)); // and store it in the dscratch1 [overwrite restore]
            write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch 
          }
          else{
            switch (size) {//Load Arg0 into S0
              case 0: 
                write32(debug_abstract, 4, lb(regnum, load_base_address, debug_data_start ));break;
              case 1: 
                write32(debug_abstract, 4, lh(regnum, load_base_address, debug_data_start ));break;
              case 2: 
                write32(debug_abstract, 4, lw(regnum, load_base_address, debug_data_start ));break;
              case 3: 
                write32(debug_abstract, 4, ld(regnum, load_base_address, debug_data_start ));break;
              }        
          }
        }
      }
      else { //CSR access
        write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
        switch (size) {//Load Arg0 into S0
          case 0: 
            write32(debug_abstract, 5, lb(S0, load_base_address, debug_data_start ));break;
          case 1: 
            write32(debug_abstract, 5, lh(S0, load_base_address, debug_data_start ));break;
          case 2: 
            write32(debug_abstract, 5, lw(S0, load_base_address, debug_data_start ));break;
          case 3: 
            write32(debug_abstract, 5, ld(S0, load_base_address, debug_data_start ));break;
        }        
        write32(debug_abstract, 6, csrw(S0, regno)); // and store it in the corresponding CSR
        write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch
      }
    }

    else if (!write)
    {
      cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an CSR/GPR/FPR Read Access command\n");
      if (!csr_access) // If GPR/FPR access
      {
        cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an GPR/FPR Read Access command\n");
        if (fpr_access) // determine whether we want to access the floating point register or not
        {
          cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an FPR Read Access command\n");
          unsigned fprnum = regno - 0x1020;
          switch(size){
           case 0:
            write32(debug_abstract, 4, fsh(fprnum, load_base_address, debug_data_start));break;
           case 1:
            write32(debug_abstract, 4, fsh(fprnum, load_base_address, debug_data_start));break;
           case 2:
            write32(debug_abstract, 4, fsw(fprnum, load_base_address, debug_data_start));break;
           case 3:
            write32(debug_abstract, 4, fsd(fprnum, load_base_address, debug_data_start));break;
          }
        }
        else
        {
          cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an GPR Read Access command\n");
          unsigned regnum = regno - 0x1000;
          cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an GPR Read Access command regnum = :{:#x}\n",regnum);

          if (regnum == 0xa){//Overwrite the Dscratch1 csr instead
            cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an GPR Read Access command for X10 reg\n");
            write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
            write32(debug_abstract, 5, csrr(S0, CSR_DSCRATCH1)); // and store it in the dscratch1 [overwrite restore]
            switch (size) {//Load Arg0 into S0
              case 0: 
                write32(debug_abstract, 6, sb(S0, load_base_address, debug_data_start ));break;
              case 1: 
                write32(debug_abstract, 6, sh(S0, load_base_address, debug_data_start ));break;
              case 2: 
                write32(debug_abstract, 6, sw(S0, load_base_address, debug_data_start ));break;
              case 3: 
                write32(debug_abstract, 6, sd(S0, load_base_address, debug_data_start ));break;
              }        
            write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch 
          }
          else{
            cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an GPR Read Access command for non-X10 reg\n");
            switch (size) {//Load Arg0 into S0
              case 0: 
                write32(debug_abstract, 4, sb(regnum, load_base_address, debug_data_start ));break;
              case 1: 
                write32(debug_abstract, 4, sh(regnum, load_base_address, debug_data_start ));break;
              case 2: 
                write32(debug_abstract, 4, sw(regnum, load_base_address, debug_data_start ));break;
              case 3: 
                write32(debug_abstract, 4, sd(regnum, load_base_address, debug_data_start ));break;
              }        
          }
        }
      }
      else { //CSR access
      cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an CSR Read Access command\n");
        write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
        write32(debug_abstract, 5, csrr(S0, regno)); // and store it in the corresponding CSR
        switch (size) {//Load Arg0 into S0
          case 0: 
            write32(debug_abstract, 6, sb(S0, load_base_address, debug_data_start ));break;
          case 1: 
            write32(debug_abstract, 6, sh(S0, load_base_address, debug_data_start ));break;
          case 2: 
            write32(debug_abstract, 6, sw(S0, load_base_address, debug_data_start ));break;
          case 3: 
            write32(debug_abstract, 6, sd(S0, load_base_address, debug_data_start ));break;
        }        
        write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch
      }
    }

    if (get_field(command, AC_ACCESS_REGISTER_POSTEXEC) && !unsupported_command) 
      // write32(debug_abstract, 9, nop());
      write32(debug_abstract, 9, jal(ZERO, debug_progbuf_start - debug_abstract_start));

    debug_rom_flags[selected_hart_id()] |= 1 << DEBUG_ROM_FLAG_GO;
    // rti_remaining = config.abstract_rti;
    // abstract_command_completed = false;
    if (!selected_hart_state().halted || !hart_available(dmcontrol.hartsel))
    {
      cvm::log(cvm::HIGH, "line 889 selected_hart_state:{:#x} hart_available:{:#x}\n",selected_hart_state().halted, hart_available(dmcontrol.hartsel));
      abstractcs.cmderr = CMDERR_HALTRESUME;
      abstractcs.busy = false;
      return true;
    }
    else { 
      cvm::log(cvm::HIGH,"Set abstractcs.busy in the perform abstract cmd\n");
      abstractcs.busy = true; 
    }
  }
  else if ((command >> 24) == 2)
  {
    //memory access
    bool aamvirtual = get_field(command, AC_ACCESS_MEMORY_AAMVIRTUAL);
    unsigned size = get_field(command, AC_ACCESS_MEMORY_AAMSIZE);
    bool aampostincrement = get_field(command, AC_ACCESS_MEMORY_AAMPOSTINCREMENT);
    bool write = get_field(command, AC_ACCESS_MEMORY_WRITE);
    
    if (size > 3) //Access size > 128 bits
    {
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }

    if (!aamvirtual)//Since we rely on core MMU, no physical access
    {
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }
    
    if (aampostincrement)//Feature unsupported
    {
      cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func\n");
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }

    debug_rom_flags[selected_hart_id()] |= 1 << DEBUG_ROM_FLAG_GO;
    if (!selected_hart_state().halted || !hart_available(dmcontrol.hartsel))
    {
      cvm::log(cvm::HIGH, "line 926 selected_hart_state:{:#x} hart_available:{:#x}\n",selected_hart_state().halted, hart_available(dmcontrol.hartsel));
      abstractcs.cmderr = CMDERR_HALTRESUME;
      abstractcs.busy = false;
      return true;
    }
    else 
      abstractcs.busy = true; 
    
    //write32(debug_abstract, 0, nop()); // store a0 in dscratch1 if it exists, but our implementation doesn't allow it
    
    (has_second_scratch)? write32(debug_abstract, 0, csrw(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 0, nop()); // store s0 in dscratch
    write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
    switch (size) {//Load Arg1 into S0
      case 0: 
        write32(debug_abstract, 5, lb(S0, load_base_address, debug_data_start + 4 ));break;
      case 1: 
        write32(debug_abstract, 5, lh(S0, load_base_address, debug_data_start + 4 ));break;
      case 2: 
        write32(debug_abstract, 5, lw(S0, load_base_address, debug_data_start + 4 ));break;
      case 3: 
        write32(debug_abstract, 5, ld(S0, load_base_address, debug_data_start + 8));break;
    }

    if(write)//Write access
    {
      //write32(debug_abstract,6,nop());
      //write32(debug_abstract,7,nop());
      switch (size) {//Load Arg0 into A0
        case 0: 
          write32(debug_abstract, 6, lb(A0, load_base_address, debug_data_start));break;
        case 1: 
          write32(debug_abstract, 6, lh(A0, load_base_address, debug_data_start));break;
        case 2: 
          write32(debug_abstract, 6, lw(A0, load_base_address, debug_data_start));break;
        case 3: 
          write32(debug_abstract, 6, ld(A0, load_base_address, debug_data_start));break;
      }


      // (size<3)? write32(debug_abstract,6,sw(S1,4,debug_data_start))    : write32(debug_abstract,6,sd(S1,8,debug_data_start));//Store S1 into Arg1
      // (size<3)? write32(debug_abstract,7,lw(S1,ZERO,debug_data_start)) : write32(debug_abstract,7,ld(S1,ZERO,debug_data_start));//Load Arg0 into S1
      write32(debug_abstract, 7, csrsi(CSR_C_PRIV, 1)); // Enable MMU Translation
      switch (size){//Store A0 data into S0 addr
        case 0 :
          write32(debug_abstract, 8, sb(S1,S0,ZERO));break;
        case 1 :
          write32(debug_abstract, 8, sh(S1,S0,ZERO));break;
        case 2 :
          write32(debug_abstract, 8, sw(S1,S0,ZERO));break;
        case 3 :
          write32(debug_abstract, 8, sd(S1,S0,ZERO));break;
      }
      write32(debug_abstract, 9, csrci(CSR_C_PRIV, 1)); // Disable MMU Translation
      write32(debug_abstract, 10, csrr(S0, CSR_DSCRATCH0)); // restore S0 again from dscratch0
      (has_second_scratch)? write32(debug_abstract, 11, csrr(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 12, nop()); // restore A0 again from dscratch1
      write32(debug_abstract,12,ebreak());
      cvm::log(cvm::HIGH, "Access Memory Write uCode update v3 \n");
    }
    else
    {
      write32(debug_abstract, 6, csrsi(CSR_C_PRIV, 1)); // Enable MMU Translation
      switch(size){
        case 0:
          write32(debug_abstract,7,lb(S0,S0,ZERO));
          write32(debug_abstract,9,sb(S0,load_base_address,debug_data_start));break;
        case 1:
          write32(debug_abstract,7,lh(S0,S0,ZERO));
          write32(debug_abstract,9,sh(S0,load_base_address,debug_data_start));break;
        case 2:
          write32(debug_abstract,7,lw(S0,S0,ZERO));
          write32(debug_abstract,9,sw(S0,load_base_address,debug_data_start));break;
        case 3:
          write32(debug_abstract,7,ld(S0,S0,ZERO));
          write32(debug_abstract,9,sd(S0,load_base_address,debug_data_start));break;
      }
      write32(debug_abstract, 8, csrci(CSR_C_PRIV, 1)); // Disable MMU Translation
      write32(debug_abstract,10, csrr(S0, CSR_DSCRATCH0)); // restore S0 again from dscratch
      (has_second_scratch)? write32(debug_abstract, 11, csrr(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 12, nop()); // restore A0 again from dscratch1
      write32(debug_abstract,12,ebreak());
      cvm::log(cvm::HIGH, "Access Memory Read uCode update v3\n");
    }

  }
  else
  {
    cvm::log(cvm::HIGH,"Setting cmderr to Notsup in the perform abs func not matching opcode\n");
    abstractcs.cmderr = CMDERR_NOTSUP;
  }

  if (!selected_hart_state().halted || !hart_available(dmcontrol.hartsel))
  {
    cvm::log(cvm::HIGH, "line 1016 selected_hart_state:{:#x} hart_available:{:#x}\n",selected_hart_state().halted, hart_available(dmcontrol.hartsel));
    abstractcs.cmderr = CMDERR_HALTRESUME;
    return true;
  }
  debug_rom_flags[hart_abscmd] = true;
  cvm::log(cvm::HIGH, "hart_abscmd:{:#x} debug_rom_flags[hart_abscmd]:{:#x}\n", hart_abscmd, debug_rom_flags[hart_abscmd]);
  return true;
}

bool debug_module_t::dmi_write(unsigned address, uint32_t value)
{
  cvm::log(cvm::HIGH, "dmi_write for addr:{:#x} with data:{:#x}\n", address, value);

  if (!dmstatus.authenticated && address != DM_AUTHDATA &&
      address != DM_DMCONTROL)
    return false;

  if (address >= DM_DATA0 && address < DM_DATA0 + abstractcs.datacount)
  {
    unsigned i = address - DM_DATA0;
    if (!abstractcs.busy)
      write32(dmdata, address - DM_DATA0, value);

    if (abstractcs.busy && abstractcs.cmderr == CMDERR_NONE)
    {
      cvm::log(cvm::HIGH, "Setting the Cmderr to Busy when DMI write to dataregs happen\n");
      abstractcs.cmderr = CMDERR_BUSY;
    }

    if (!abstractcs.busy && ((abstractauto.autoexecdata >> i) & 1))
    {
      perform_abstract_command();
    }
    return true;
  }
  else if (address >= DM_PROGBUF0 && address < DM_PROGBUF0 + config.progbufsize)
  {
    unsigned i = address - DM_PROGBUF0;

    if (!abstractcs.busy)
      write32(program_buffer.data(), config.support_impebreak?(i+1):i , value);

    if (!abstractcs.busy && ((abstractauto.autoexecprogbuf >> i) & 1))
    {
      perform_abstract_command();
    }
    return true;
  }
  else
  {
    switch (address)
    {
    case DM_DMCONTROL:
    {
      cvm::log(cvm::HIGH, "Inside the DMCONTROL Write Event - 001 with num harts:{:#x}\n", harts.size());
      if (!dmcontrol.dmactive && get_field(value, DM_DMCONTROL_DMACTIVE))
        reset();
      dmcontrol.dmactive = get_field(value, DM_DMCONTROL_DMACTIVE);
      if (!dmstatus.authenticated || !dmcontrol.dmactive)
        return true;

      dmcontrol.haltreq = get_field(value, DM_DMCONTROL_HALTREQ);
      dmcontrol.resumereq = get_field(value, DM_DMCONTROL_RESUMEREQ);
      dmcontrol.hartreset = get_field(value, DM_DMCONTROL_HARTRESET);
      dmcontrol.ndmreset = get_field(value, DM_DMCONTROL_NDMRESET);
      if (config.support_hasel)
        dmcontrol.hasel = get_field(value, DM_DMCONTROL_HASEL);
      else
        dmcontrol.hasel = 0;
      hartsel = get_field(value, DM_DMCONTROL_HARTSELHI) << DM_DMCONTROL_HARTSELLO_LENGTH;
      hartsel |= get_field(value, DM_DMCONTROL_HARTSELLO);
      if (hartsel < max_hartid) {
        dmcontrol.hartsel = hartsel;
      } else {
        dmcontrol.hartsel = 0;
      }
      //dmcontrol.hartsel = std::min(size_t(dmcontrol.hartsel), max_hartid - 1); //FIXME
      // dmcontrol.hartsel = max_hartid - 1;

      for (const auto &[hart_id, hart] : harts)
      {
        cvm::log(cvm::HIGH, "Inside for loop for hart_idx = {:#x}\n", hart_id);
        if (hart_selected(hart_id))
        {
          cvm::log(cvm::HIGH, "Inside the DMCONTROL Write Event - 002\n");
          if (get_field(value, DM_DMCONTROL_ACKHAVERESET))
          {
            dmcontrol.ackhavereset = false;
            hart_state[hart_id].havereset = false;
          }
          if (dmcontrol.haltreq && hart_available(hart_id) && !dmcontrol.ndmreset)
          {
            hart->halt_request = hart->HR_REGULAR;
            dmcontrol.haltreq = false;
            cvm::log(cvm::HIGH, "halt hart: {:#x}\n", hart_id);
            if(hart_state[hart_id].haltgroup) 
            {
              hart_haltreq_hg = dmcontrol.hartsel;
              hartsel_stable = true;
            }
            for (const auto &[hart_id1, hart] : harts)
            {
              if (!hart_state[hart_id1].halted &&
                  hart_state[hart_id1].haltgroup == hart_state[hart_id].haltgroup &&
                  hart_available(hart_id1) && (hart_id1 != hart_id))
              {
                cvm::log(cvm::HIGH, "before debug_rom_flags haltgrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                debug_rom_flags[hart_id1] |= (1 << DEBUG_ROM_FLAG_HALTGRP);
                cvm::log(cvm::HIGH, "debug_rom_flags haltgrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                hart->halt_request = hart->HR_GROUP;
              }
              else if (!hart_state[hart_id1].halted &&
                hart_state[hart_id1].haltgroup == hart_state[hart_id].haltgroup &&
                hart_available(hart_id1) && (hart_id1 == hart_id)) 
              {
                cvm::log(cvm::HIGH, "before debug_rom_flags haltgrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                debug_rom_flags[hart_id1] = 0;
                cvm::log(cvm::HIGH, "debug_rom_flags haltgrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                hart->halt_request = hart->HR_GROUP;
              }
            }
          }
          else if (dmcontrol.ndmreset && hart_available(hart_id))
          {
            dmstatus.ndmresetpending = true;
            cvm::log(cvm::HIGH, "Ndmreset and pending is 1 in dm_model\n");
            hart->reset();
            hart->halt_request = hart->HR_NONE;
            hart_state[hart_id].resumeack = false;
            ndm_reset_assert = true;
            hart_state[hart_id].havereset = true;
          //  dmstatus.allrunning = true;
          //  dmstatus.anyrunning = true;
          //  dmstatus.allhalted = false;
          //  dmstatus.anyhalted = false;
          }
          else
          {
            hart->halt_request = hart->HR_NONE;
          }
          if (dmcontrol.resumereq && hart_available(hart_id))
          {
            cvm::log(cvm::HIGH, "resume hart: {:#x}", hart_id);
            // debug_rom_flags[hart_id] |= (1 << DEBUG_ROM_FLAG_RESUME);
            for (const auto &[hart_id1, hart] : harts)
              {
                if (hart_state[hart_id1].halted &&
                    hart_state[hart_id1].resumegroup == hart_state[hart_id].resumegroup &&
                    hart_available(hart_id1))
                {
                  cvm::log(cvm::HIGH, "before debug_rom_flags resumegrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                   // debug_rom_flags[hart_id1] |= (1 << DEBUG_ROM_FLAG_RESUME);
                  debug_rom_flags[hart_id1] = 0x2;
                  cvm::log(cvm::HIGH, "debug_rom_flags resumegrp set :{:#x}, :{:#x}\n", hart_id1, debug_rom_flags[hart_id1]);
                }
              }
            // cvm::log(cvm::HIGH, "debug_rom_flags resume set :{:#x}, :{:#x}\n", hart_id, debug_rom_flags[hart_id]);
            hart_state[hart_id].resumeack = false;
            dmcontrol.resumereq = false;
          }
          if (dmcontrol.hartreset && hart_available(hart_id))
          {
            hart->reset();
          }
        }
        else
        {
          cvm::log(cvm::FULL, "Not entering as hart_selected for Hart ID is false");
        }
        if (!dmcontrol.ndmreset && ndm_reset_assert)
        {
          dmstatus.ndmresetpending = false;
          cvm::log(cvm::HIGH, "Ndmreset and pending is 0 in dm_model\n");
          ndm_reset_assert = false;
        }
      }

      if ((hart_haltreq_hg != dmcontrol.hartsel) && hartsel_stable) 
      {
        cvm::log(cvm::HIGH,"if hartsel_stable:{:#x}, hart_haltreq_hg:{:#x}, dmcontrol.hartsel:{:#x} \n",hartsel_stable, hart_haltreq_hg, dmcontrol.hartsel);
        hartsel_stable = false;
        for (const auto &[hart_id, hart] : harts)
        {
          if(hartsel_stable == false) 
          {
            cvm::log(cvm::HIGH, " before debug_rom_flags haltgrp clear hart_id:{:#x}, debug_rom_flags[hart_id]:{:#x}\n", hart_id, debug_rom_flags[hart_id]);
            //debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_HALTGRP);
            debug_rom_flags[hart_id] = 0;
            cvm::log(cvm::HIGH, "debug_rom_flags haltgrp clear hart_id:{:#x}, debug_rom_flags[hart_id]:{:#x}\n", hart_id, debug_rom_flags[hart_id]);
          }
        }
      }
      else 
      {
        cvm::log(cvm::HIGH,"else hartsel_stable:{:#x}, hart_haltreq_hg:{:#x}, dmcontrol.hartsel:{:#x} \n",hartsel_stable, hart_haltreq_hg, dmcontrol.hartsel);
      }
// if (dmcontrol.ndmreset)
// {
// dmstatus.ndmresetpending = true;
// for (const auto &[hart_id, hart] : harts)
// {
// if (hart_selected(hart_id))
// {
// cvm::log(cvm::FULL, "Ndmreset and pending is 1 in dm_model\n");
// hart->reset();
// hart->halt_request = hart->HR_NONE;
// hart_state[hart_id].resumeack = false;
// ndm_reset_assert = true;
// hart_state[hart_id].havereset = true;
// }
// }
// }
// else if(!dmcontrol.ndmreset && ndm_reset_assert)
// {
// dmstatus.ndmresetpending = false;
// ndm_reset_assert = false;
// }
    }
      return true;

    case DM_COMMAND:
      command = value;
      return perform_abstract_command();

      case DM_HAWINDOWSEL:
        hawindowsel = value & ((1U<<field_width(hart_array_mask.size()))-1);
        return true;

      case DM_HAWINDOW:
        {
          unsigned base = hawindowsel * 32;
          for (unsigned i = 0; i < 32; i++) {
            unsigned n = base + i;
            if (n < max_hartid) {
              hart_array_mask[n] = (value >> i) & 1;
            }
          }
        }
        return true;

    case DM_ABSTRACTCS:
      cvm::log(cvm::HIGH, "Write to Abstract cmderr with val of {:#x}\n", (uint32_t)(get_field(value, DM_ABSTRACTCS_CMDERR)));
      cvm::log(cvm::HIGH, "[Check] cmderr before the write is {:#x}\n", (uint32_t)(abstractcs.cmderr));
      cvm::log(cvm::HIGH, "[Weirdness here] neg_field val = {:#x}, calc cal = {:#x}\n",(~(uint32_t)(get_field(value, DM_ABSTRACTCS_CMDERR))), (((uint32_t)(abstractcs.cmderr)) & (~(uint32_t)(get_field(value, DM_ABSTRACTCS_CMDERR)))));
      abstractcs.cmderr = (cmderr_t)(((uint32_t)(abstractcs.cmderr)) & (~(uint32_t)(get_field(value, DM_ABSTRACTCS_CMDERR))));
      cvm::log(cvm::HIGH, "[Check] cmderr after the write is {:#x}\n", (uint32_t)(abstractcs.cmderr));
      return true;

    case DM_ABSTRACTAUTO:
      abstractauto.autoexecprogbuf = get_field(value,
                                               DM_ABSTRACTAUTO_AUTOEXECPROGBUF);
      abstractauto.autoexecdata = get_field(value,
                                            DM_ABSTRACTAUTO_AUTOEXECDATA);
      return true;
      // case DM_AUTHDATA:
      //   D(fprintf(stderr, "debug authentication: got 0x%x; 0x%x unlocks\n", value,
      //       challenge + secret));
      //   if (config.require_authentication) {
      //     if (value == challenge + secret) {
      //       dmstatus.authenticated = true;
      //     } else {
      //       dmstatus.authenticated = false;
      //       challenge = random();
      //     }
      //   }
      //   return true;

      case DM_DMCS2:
      dmcs2.grouptype = get_field(value, DM_DMCS2_GROUPTYPE);
      dmcs2.dmexttrigger = 0; //get_field(value, DM_DMCS2_DMEXTTRIGGER);
      dmcs2.hgwrite = get_field(value, DM_DMCS2_HGWRITE);
      dmcs2.hgselect = 0; //get_field(value, DM_DMCS2_HGSELECT);
        if (config.support_haltgroups && dmcs2.hgwrite) {
          cvm::log(cvm::HIGH, "[DM Model] Updating group for halt/resume. hgwrite is :{:#x}\n", dmcs2.hgwrite);
          if (get_field(value, DM_DMCS2_GROUPTYPE) == 0) {
            selected_hart_state().haltgroup = get_field(value, DM_DMCS2_GROUP) & 0x1;
            dmcs2.group = get_field(value, DM_DMCS2_GROUP) & 0x1;
            cvm::log(cvm::HIGH, "dmcs2 programming haltgrp set hart id :{:#x}, :{:#x}\n", selected_hart_id(), selected_hart_state().haltgroup);
          }
          else if (get_field(value, DM_DMCS2_GROUPTYPE) == 1){
            selected_hart_state().resumegroup = get_field(value, DM_DMCS2_GROUP) & 0x1;
            dmcs2.group = get_field(value, DM_DMCS2_GROUP) & 0x1;
            cvm::log(cvm::HIGH, "dmcs2 programming resumegrp set hart id :{:#x}, :{:#x}\n", selected_hart_id(), selected_hart_state().resumegroup);
          }
      }
      return true;   
      
      case DM_HALTSUM0:
      return true;   
  }
  return false;
}
}

void debug_module_t::proc_reset(unsigned id)
{
  hart_state[id].havereset = true;
  hart_state[id].halted = false;
  hart_state[id].haltgroup = 0;
  hart_state[id].resumegroup = 0;
}

hart_debug_state_t &debug_module_t::selected_hart_state()
{
  return hart_state[selected_hart_id()];
}

size_t debug_module_t::selected_hart_id() const
{ // Add support for the fuse map to get the actual VID
  return dmcontrol.hartsel;
}
