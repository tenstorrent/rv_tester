#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "dm_model.hpp"

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

REGISTRY_register(debug_module_t, TOP.PLATFORM.DM_MODEL, 0);

debug_module_t::debug_module_t(cvm::topology::loc_t loc, unsigned) : program_buffer_bytes((config.support_impebreak ? 4 : 0) + 4 * config.progbufsize),
                                                                     debug_progbuf_start(debug_data_start - program_buffer_bytes),
                                                                     debug_abstract_start(debug_progbuf_start - debug_abstract_size * 4),
                                                                    //  custom_base(0),
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

  // Define a processor array (for the number of harts)
  for (size_t i = 0; i < max_hartid; i++)
  {
    harts[i] = std::make_unique<processor_t>(halt_on_reset);
  }

  program_buffer = std::vector<uint8_t>(program_buffer_bytes, 0);

  memset(debug_rom_flags, 0, sizeof(debug_rom_flags));
  memset(dmdata, 0, sizeof(dmdata));

  write32(debug_rom_whereto, 0,
          jal(ZERO, debug_abstract_start - DEBUG_ROM_WHERETO));

  memset(debug_abstract, 0, sizeof(debug_abstract));
  for (unsigned i = 0; i < max_hartid; i++)
  {
    hart_available_state[i] = true;
  }

  reset();
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dmi_req<> &dmi_req)
{
  uint32_t read_value;
  cvm::log(cvm::HIGH, "Model recieved dmi request: op {:#x} addr {:#x}\n", dmi_req.op, dmi_req.addr);
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
  cvm::log(cvm::HIGH, "Seen a response with data: {:#x} and prev req expected:{:#x}\n", dmi_resp.data, req_expect);
  uint32_t actual_data = dmi_resp.data;

  if (req_resp_check)
  {
    if (actual_data != req_expect)
      cvm::log(cvm::ERROR, "[Error-Mismatch] Seen a DMI Response Mismatch for Addr:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n", reg_addr_to_check, actual_data, req_expect);
    req_resp_check = false;
    return;
  }
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_load_cmd<> &dm_load_cmd)
{
  load_req_length = std::ceil(pow(2, dm_load_cmd.size) / 8);

  cvm::log(cvm::HIGH, "[{}] Sent a load req for the address:{:#x} for length:{:#x} with id:{:#x}\n", sent_count, dm_load_cmd.addr, load_req_length, dm_load_cmd.id);
  sent_count++;

  debug_module_t::load(dm_load_cmd.addr, load_req_length, load_bytes_data);

  std::memcpy(&expected_load_data, load_bytes_data, load_req_length);
  cvm::log(cvm::HIGH, "The Load data expected for id:{:#x}, is data:{:#x}\n", dm_load_cmd.id, expected_load_data);

  load_req_id = dm_load_cmd.id;
  load_req_addr = dm_load_cmd.addr;
  mem_load_check = true;
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_load_data<> &dm_load_data)
{
  uint64_t sizer_slice_pos = cvm::bitmanip::slice<uint64_t>(load_req_addr, 5, 3);
  for (int i=0;i<64;i++){
      sizer_slice_data[i] = dm_load_data.data[sizer_slice_pos*64+i];
    }
  cvm::log(cvm::HIGH, "[{}] Got a load resp for the id:{:#x} and data:{:#x}\n", resp_count, dm_load_data.id, sizer_slice_data.to_ullong());
  resp_count++;

  if (load_req_id == dm_load_data.id)
  {
    cvm::log(cvm::HIGH, "Seen a matching load response for the same id as the previous load request\n");
    uint64_t expected_load_data_to_check = cvm::bitmanip::slice<uint64_t>(expected_load_data, (load_req_length * 4 - 1), 0);
    
    

    uint64_t actual_load_data_to_check = cvm::bitmanip::slice<uint64_t>(sizer_slice_data.to_ullong(), (load_req_length * 4 - 1), 0);
    
    if (expected_load_data_to_check != actual_load_data_to_check){
      if(load_req_addr==0x400 & reflow_flags==0 ){
        reflow_flags=1; 
        cvm::log(cvm::HIGH, "Reflowing 0x400 read to allow DM to update state flag\n");
        //#FIXME : If a cleaner way is possible
      }
      else {
        reflow_flags=0;
        cvm::log(cvm::ERROR, "[Error-Mismatch] The load data's are mismatching for Addr:{:#x} with Length:{:#x} ~~~ Actual:{:#x} vs Expected:{:#x}\n",load_req_addr,load_req_length,actual_load_data_to_check,expected_load_data_to_check);
      }
      
    }
  }
}

void debug_module_t::process(const rv_tester_transactions::dm_model::dm_store<> &dm_store)
{
  cvm::log(cvm::HIGH, "Seen a store req for the address:{:#x} and len:{:#x} and data:{:#x}\n", dm_store.addr, dm_store.len, dm_store.data);

  uint8_t *store_data = (uint8_t *)&dm_store.data;
  debug_module_t::store(dm_store.addr, 4, store_data);
}

void debug_module_t::reset()
{
  for (const auto &[hart_id, hart] : harts)
  { // harts
    hart->halt_request = hart->HR_NONE;
  }

  memset(&dmcontrol, 0, sizeof(dmcontrol));

  memset(&dmstatus, 0, sizeof(dmstatus));
  dmstatus.impebreak = config.support_impebreak;
  dmstatus.authenticated = !config.require_authentication;
  dmstatus.version = 2;

  memset(&abstractcs, 0, sizeof(abstractcs));
  abstractcs.datacount = sizeof(dmdata) / 4;
  abstractcs.progbufsize = config.progbufsize;

  memset(&abstractauto, 0, sizeof(abstractauto));

  for (auto &ele : hart_state)
  { // Add havereset for all the core
    ele.havereset = true;
  }

  cvm::log(cvm::HIGH, "[Config] debug_data_start={:#x}\n", 0x380); //Fixed value as per the implementation
  cvm::log(cvm::HIGH, "[Config] debug_progbuf_start={:#x}\n", debug_progbuf_start);
  cvm::log(cvm::HIGH, "[Config] debug_abstract_start={:#x}\n", debug_abstract_start);

  unsigned i = 1;
  (has_second_scratch)? write32(debug_abstract,i++,auipc(A0,ZERO)) : write32(debug_abstract, i++, nop());    // 0 (high)
  (has_second_scratch)? write32(debug_abstract,i++,srli(A0,A0,12)) : write32(debug_abstract, i++, nop());    // 1 (low)
  (has_second_scratch)? write32(debug_abstract,i++,slli(A0,A0,12)) : write32(debug_abstract, i++, nop());    // 1 (high)
  write32(debug_abstract, i++, nop());    // 2 (low)
  write32(debug_abstract, i++, nop());    // 2 (high)
  write32(debug_abstract, i++, nop());    // 3 (low)
  write32(debug_abstract, i++, nop());    // 3 (high)
  (has_second_scratch)? write32(debug_abstract,i++,csrr(A0,CSR_DSCRATCH1)) : write32(debug_abstract, i++, nop());    // 4 (low)
  write32(debug_abstract, i++, ebreak()); // 4 (high)
  
}

bool debug_module_t::load(reg_t addr, size_t len, uint8_t *bytes)
{
  if (len == 0)
    len = 1;
  addr = DEBUG_START + addr;
  cvm::log(cvm::HIGH, "Load Request Addr:{:#x}, len:{:#x} first_range:{:#x}\n", addr, len, (DEBUG_ROM_ENTRY + debug_rom_raw_len));

  if (addr >= DEBUG_ROM_ENTRY) //&& (addr + len) <= (DEBUG_ROM_ENTRY + debug_rom_raw_len))
  {
    if ((addr + len - DEBUG_ROM_ENTRY) > debug_rom_raw_len)
    {
      addr = ((addr >> 3) & 0x0F) * 8; // addr - debug_rom_raw_len;
      memcpy(bytes, debug_rom_raw + addr, len);
    }
    else
      memcpy(bytes, debug_rom_raw + addr - DEBUG_ROM_ENTRY, len);
    return true;
  }

  if (addr >= DEBUG_ROM_WHERETO && (addr + len) <= (DEBUG_ROM_WHERETO + 8)) // FIXME: Check if this should be 4 or 8
  {
    cvm::log(cvm::HIGH, "Whereto Case ::: Addr={:#x}, Length={:#x}\n",addr,len);
    memcpy(bytes, debug_rom_whereto + addr - DEBUG_ROM_WHERETO, len);
    return true;
  }

  if (addr >= DEBUG_ROM_FLAGS && ((addr + len) <= DEBUG_ROM_FLAGS + 1024))
  {
    memcpy(bytes, debug_rom_flags + addr - DEBUG_ROM_FLAGS, len);
    return true;
  }

  if (addr >= debug_abstract_start && ((addr + len) <= (debug_abstract_start + sizeof(debug_abstract))))
  {
    memcpy(bytes, debug_abstract + addr - debug_abstract_start, len);
    return true;
  }

  if (addr >= debug_data_start && (addr + len) <= (debug_data_start + sizeof(dmdata)))
  {
    memcpy(bytes, dmdata + addr - debug_data_start, len);
    return true;
  }

  if (addr >= debug_progbuf_start && ((addr + len) <= (debug_progbuf_start + program_buffer_bytes)))
  {
    memcpy(bytes, program_buffer.data() + addr - debug_progbuf_start, len);
    return true;
  }

  cvm::log(cvm::ERROR, "ERROR: invalid load from debug module --> len:{:#x} at addr:{:#x}\n", len, addr);

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
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Halted State\n");
    assert(len == 4);
    if (!hart_state[id].halted)
    {
      hart_state[id].halted = true;
      if (hart_state[id].haltgroup)
      {
        for (const auto &[hart_id, hart] : harts)
        {
          if (!hart_state[hart_id].halted &&
              hart_state[hart_id].haltgroup == hart_state[id].haltgroup &&
              hart_available(hart_id))
          {
            hart->halt_request = hart->HR_GROUP;
            // TODO: What if the debugger comes and writes dmcontrol before the
            // halt occurs?
          }
        }
      }
    }
    if (selected_hart_id() == id)
    {
      if (0 == (debug_rom_flags[id] & (1 << DEBUG_ROM_FLAG_GO)))
      {
        // abstract_command_completed = true;
        abstractcs.busy = false;
      }
    }
    return true;
  }

  if (addr == DEBUG_ROM_GOING)
  {
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Going State\n");
    assert(len == 4);
    debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_GO);
    return true;
  }

  if (addr == DEBUG_ROM_RESUMING)
  {
    cvm::log(cvm::HIGH, "In the DEBUG_ROM Resume State\n");
    assert(len == 4);
    hart_state[id].halted = false;
    hart_state[id].resumeack = true;
    debug_rom_flags[id] &= ~(1 << DEBUG_ROM_FLAG_RESUME);
    return true;
  }

  if (addr == DEBUG_ROM_EXCEPTION)
  {
    if (abstractcs.cmderr == CMDERR_NONE)
    {
      abstractcs.cmderr = CMDERR_EXCEPTION;
    }
    return true;
  }

  cvm::log(cvm::ERROR, "ERROR: invalid store to debug module --> len:{:#x} at addr:{:#x}\n", len, addr);

  return false;
}

void debug_module_t::write32(uint8_t *memory, unsigned int index, uint32_t value)
{
  uint8_t *base = memory + index * 4;
  cvm::log(cvm::HIGH, "Write32 called for index:{:#x} at memory loc:{:#x} with value:{:#x}\n", index, *base, value);
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
  return hartid == selected_hart_id() || (dmcontrol.hasel && hart_array_mask[hartid]);
}

bool debug_module_t::hart_available(unsigned hart_id) const
{
  if (hart_id < max_hartid)
    return hart_available_state[hart_id];
  return true;
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
    result = read32(program_buffer.data(), i);
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
          dmstatus.allnonexistant = false;
          if (hart_state[hart_id].resumeack)
          {
            dmstatus.anyresumeack = true;
          }
          else
          {
            dmstatus.allresumeack = false;
          }
          // auto hart = harts.at(hart_id);
          if (hart_state[hart_id].halted)
          {
            dmstatus.allrunning = false;
            dmstatus.anyhalted = true;
            dmstatus.allunavail = false;
          }
          else if (!hart_available(hart_id))
          {
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
      // dmstatus.anynonexistant = dmcontrol.hartsel >= sim->get_cfg().nprocs();

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
    default:
      result = 0;
      cvm::log(cvm::ERROR, "Unexpected Register [Maybe not part of this implementation]. Returning Error.\n");
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
  
  cvm::log(cvm::HIGH, "[Abstract Cmd] Performing an abstract command\n");
  if (abstractcs.cmderr != CMDERR_NONE)
    return true;
  if (abstractcs.busy)
  {
    abstractcs.cmderr = CMDERR_BUSY;
    return true;
  }

  if (!selected_hart_state().halted)
    {
      abstractcs.cmderr = CMDERR_HALTRESUME;
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

    if (size > 3)
    {
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }

    if (postexec==0 && transfer==0) // Implementation does not support this config and marks it as unsupported
    {
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }

    //** Custom logic to meet the implementation specific details
    if ((size <= 4) && transfer && write)
    { // Check for access reg type
      write32(debug_abstract, 0, nop()); // store a0 in dscratch1 if it exists, but our implementation doesn't allow it

      if (cvm::bitmanip::slice<uint64_t>(regno, 15, 14) != 0){
        write32(debug_abstract, 0, ebreak()); // Command not supported
        unsupported_command = true;
      }
    
      else if (cvm::bitmanip::slice<uint64_t>(regno, 12, 12)) // GPR/FPR access
      {
        if (cvm::bitmanip::slice<uint64_t>(regno, 5, 5)) // determine whether we want to access the floating point register or not
        {
          unsigned fprnum = regno - 0x1020;
          if (size == 2)
            write32(debug_abstract, 4, flw(fprnum, ZERO, debug_data_start));
          else if (size == 4)
            write32(debug_abstract, 4, fld(fprnum, ZERO, debug_data_start));
        }
        else
        {
          unsigned regnum = regno - 0x1000;
          if (size == 2)
            write32(debug_abstract, 4, lw(regnum, ZERO, debug_data_start));
          else if (size == 4)
            write32(debug_abstract, 4, ld(regnum, ZERO, debug_data_start));
        }
      }

      else { //CSR access 
        write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
        if (size == 2)
            write32(debug_abstract, 5, lw(S0, ZERO, debug_data_start)); // load from data register
        else if (size == 4)
            write32(debug_abstract, 5, ld(S0, ZERO, debug_data_start)); // load from data register
        write32(debug_abstract, 6, csrw(S0, regno)); // and store it in the corresponding CSR
        write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch
      }
    }

    else if ((size <= 4) && transfer && !write)
    {
      write32(debug_abstract, 0, nop()); // store a0 in dscratch1 if there's second scratch, but our impl doesnt have it so nop()

      if (cvm::bitmanip::slice<uint64_t>(regno, 15, 14) != 0){
        write32(debug_abstract, 0, ebreak()); // Command not supported
        unsupported_command = true;
      }
      
      else if (cvm::bitmanip::slice<uint64_t>(regno, 12, 12)){ // GPR/FPR access
        if (cvm::bitmanip::slice<uint64_t>(regno, 5, 5)) // determine whether we want to access the floating point register or not
        {
          unsigned fprnum = regno - 0x1020;
          if (size == 2)
            write32(debug_abstract, 4, fsw(fprnum, ZERO, debug_data_start));
          else if (size == 4)
            write32(debug_abstract, 4, fsd(fprnum, ZERO, debug_data_start));
        }
        else
        {
          unsigned regnum = regno - 0x1000;
          if (size == 2)
            write32(debug_abstract, 4, sw(regnum, ZERO, debug_data_start));
          else if (size == 4)
            write32(debug_abstract, 4, sd(regnum, ZERO, debug_data_start));
        }
      }

      else { //CSR access
        write32(debug_abstract, 4, csrw(S0, CSR_DSCRATCH0)); // store s0 in dscratch
        write32(debug_abstract, 5, csrr(S0, regno)); // read value from CSR into s0
        if (size == 2)
            write32(debug_abstract, 6, lw(S0, ZERO, debug_data_start)); // and store s0 into data section
        else if (size == 4)
            write32(debug_abstract, 6, ld(S0, ZERO, debug_data_start)); // and store s0 into data section
        write32(debug_abstract, 7, csrr(S0, CSR_DSCRATCH0)); // restore s0 again from dscratch
      }
    }
    else if ( size > 4 || get_field(command, AC_ACCESS_REGISTER_AARPOSTINCREMENT)){
      write32(debug_abstract, 0, ebreak());
      unsupported_command = true;
    }

    if (get_field(command, AC_ACCESS_REGISTER_POSTEXEC) && !unsupported_command) 
      // write32(debug_abstract, 9, nop());
      write32(debug_abstract, 9, jal(ZERO, debug_progbuf_start - debug_abstract_start));

    //** End of the custom specified logic

    // unsigned i = 0;
    // if (get_field(command, AC_ACCESS_REGISTER_TRANSFER))
    // {

    // if (is_fpu_reg(regno))
    //   {
    //     // Save S0
    //     write32(debug_abstract, i++, csrw(S0, CSR_DSCRATCH0));
    //     // Save mstatus
    //     write32(debug_abstract, i++, csrr(S0, CSR_MSTATUS));
    //     write32(debug_abstract, i++, csrw(S0, CSR_DSCRATCH1));
    //     // Set mstatus.fs
    //     assert((MSTATUS_FS & 0xfff) == 0);
    //     write32(debug_abstract, i++, lui(S0, MSTATUS_FS >> 12));
    //     write32(debug_abstract, i++, csrrs(ZERO, S0, CSR_MSTATUS));
    //   }

    //   if (regno < 0x1000 && config.support_abstract_csr_access)
    //   {
    //     if (!is_fpu_reg(regno))
    //     {
    //       write32(debug_abstract, i++, csrw(S0, CSR_DSCRATCH0));
    //       cvm::log(cvm::NONE, "Doing ----------000000000000\n");
    //     }

    //     if (write)
    //     {
    //       cvm::log(cvm::NONE, "Doing 000000000000\n");
    //       switch (size)
    //       {
    //       case 2:
    //         write32(debug_abstract, i++, lw(S0, ZERO, debug_data_start));
    //         break;
    //       case 3:
    //         write32(debug_abstract, i++, ld(S0, ZERO, debug_data_start));
    //         break;
    //       default:
    //         abstractcs.cmderr = CMDERR_NOTSUP;
    //         return true;
    //       }
    //       write32(debug_abstract, i++, csrw(S0, regno));
    //     }
    //     else
    //     {
    //       cvm::log(cvm::NONE, "Doing 11111111111111\n");
    //       write32(debug_abstract, i++, csrr(S0, regno));
    //       switch (size)
    //       {
    //       case 2:
    //         write32(debug_abstract, i++, sw(S0, ZERO, debug_data_start));
    //         break;
    //       case 3:
    //         write32(debug_abstract, i++, sd(S0, ZERO, debug_data_start));
    //         break;
    //       default:
    //         abstractcs.cmderr = CMDERR_NOTSUP;
    //         return true;
    //       }
    //     }
    //     if (!is_fpu_reg(regno))
    //     {
    //       write32(debug_abstract, i++, csrr(S0, CSR_DSCRATCH0));
    //     }
    //   }
    //   else if (regno >= 0x1000 && regno < 0x1020)
    //   {
    //     unsigned regnum = regno - 0x1000;

    //     switch (size)
    //     {
    //     case 2:
    //       if (write)
    //         write32(debug_abstract, i++, lw(regnum, ZERO, debug_data_start));
    //       else
    //         write32(debug_abstract, i++, sw(regnum, ZERO, debug_data_start));
    //       break;
    //     case 3:
    //       if (write)
    //         write32(debug_abstract, i++, ld(regnum, ZERO, debug_data_start));
    //       else
    //         write32(debug_abstract, i++, sd(regnum, ZERO, debug_data_start));
    //       break;
    //     default:
    //       abstractcs.cmderr = CMDERR_NOTSUP;
    //       return true;
    //     }

    //     if (regno == 0x1000 + S0 && write)
    //     {
    //       /*
    //        * The exception handler starts out be restoring dscratch to s0,
    //        * which was saved before executing the abstract memory region. Since
    //        * we just wrote s0, also make sure to write that same value to
    //        * dscratch in case an exception occurs in a program buffer that
    //        * might be executed later.
    //        */
    //       write32(debug_abstract, i++, csrw(S0, CSR_DSCRATCH0));
    //     }
    //   }
    //   else if (regno >= 0x1020 && regno < 0x1040 && config.support_abstract_fpr_access)
    //   {
    //     unsigned fprnum = regno - 0x1020;

    //     if (write)
    //     {
    //       switch (size)
    //       {
    //       case 2:
    //         write32(debug_abstract, i++, flw(fprnum, ZERO, debug_data_start));
    //         break;
    //       case 3:
    //         write32(debug_abstract, i++, fld(fprnum, ZERO, debug_data_start));
    //         break;
    //       default:
    //         abstractcs.cmderr = CMDERR_NOTSUP;
    //         return true;
    //       }
    //     }
    //     else
    //     {
    //       switch (size)
    //       {
    //       case 2:
    //         write32(debug_abstract, i++, fsw(fprnum, ZERO, debug_data_start));
    //         break;
    //       case 3:
    //         write32(debug_abstract, i++, fsd(fprnum, ZERO, debug_data_start));
    //         break;
    //       default:
    //         abstractcs.cmderr = CMDERR_NOTSUP;
    //         return true;
    //       }
    //     }
    //   }
    //   else if (regno >= 0xc000 && (regno & 1) == 1)
    //   {
    //     // Support odd-numbered custom registers, to allow for debugger testing.
    //     unsigned custom_number = regno - 0xc000;
    //     abstractcs.cmderr = CMDERR_NONE;
    //     if (write)
    //     {
    //       // Writing V to custom register N will cause future reads of N to
    //       // return V, reads of N-1 will return V-1, etc.
    //       custom_base = read32(dmdata, 0) - custom_number;
    //     }
    //     else
    //     {
    //       write32(dmdata, 0, custom_number + custom_base);
    //       write32(dmdata, 1, 0);
    //     }
    //     return true;
    //   }
    //   else
    //   {
    //     abstractcs.cmderr = CMDERR_NOTSUP;
    //     return true;
    //   }

    //   if (is_fpu_reg(regno))
    //   {
    //     // restore mstatus
    //     write32(debug_abstract, i++, csrr(S0, CSR_DSCRATCH1));
    //     write32(debug_abstract, i++, csrw(S0, CSR_MSTATUS));
    //     // restore s0
    //     write32(debug_abstract, i++, csrr(S0, CSR_DSCRATCH0));
    //   }
    // }

    // if (get_field(command, AC_ACCESS_REGISTER_POSTEXEC))
    // {
    //   cvm::log(cvm::NONE, "Doing 222222222222\n");
    //   write32(debug_abstract, i,
    //           jal(ZERO, debug_progbuf_start - debug_abstract_start - 4 * i));
    //   i++;
    // }
    // else
    // {
    //   cvm::log(cvm::NONE, "Doing 333333333333\n");
    //   write32(debug_abstract, i++, ebreak());
    // }

    debug_rom_flags[selected_hart_id()] |= 1 << DEBUG_ROM_FLAG_GO;
    // rti_remaining = config.abstract_rti;
    // abstract_command_completed = false;

    abstractcs.busy = true;
  }
  else if ((command >> 24) == 2)
  {
    //memory access
    bool aamvirtual = get_field(command, AC_ACCESS_MEMORY_AAMVIRTUAL);
    unsigned size = get_field(command, AC_ACCESS_MEMORY_AAMSIZE);
    bool aampostincrement = get_field(command, AC_ACCESS_MEMORY_AAMPOSTINCREMENT);
    bool write = get_field(command, AC_ACCESS_MEMORY_WRITE);
    // bool unsupported_command = false;
    
    if (size > 3) //Access size > 128 bits
    {
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }

    if (!aamvirtual)//Since we rely on core MMU, no physical access
    {
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }
    
    if (aampostincrement)//Feature unsupported
    {
      abstractcs.cmderr = CMDERR_NOTSUP;
      return true;
    }
    debug_rom_flags[selected_hart_id()] |= 1 << DEBUG_ROM_FLAG_GO;
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
      switch (size){//Store A0 data into S0 addr
        case 0 :
          write32(debug_abstract, 7, sb(A0,S0,ZERO));break;
        case 1 :
          write32(debug_abstract, 7, sh(A0,S0,ZERO));break;
        case 2 :
          write32(debug_abstract, 7, sw(A0,S0,ZERO));break;
        case 3 :
          write32(debug_abstract, 7, sd(A0,S0,ZERO));break;
      }
      write32(debug_abstract, 8, csrr(S0, CSR_DSCRATCH0)); // restore S0 again from dscratch0
      (has_second_scratch)? write32(debug_abstract, 9, csrr(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 9, nop()); // restore A0 again from dscratch1
      // (size<3)? write32(debug_abstract,11,ld(S1,4,debug_data_start))    : write32(debug_abstract,11,ld(S1,8,debug_data_start));//Restore Arg1 into S1
      // write32(debug_abstract,12,nop());
      write32(debug_abstract,10,ebreak());
      cvm::log(cvm::HIGH, "Access Memory Write uCode update v2 \n");
    }
    else
    {
      switch(size){
        case 0:
          write32(debug_abstract,6,lb(S0,S0,ZERO));
          write32(debug_abstract,7,sb(S0,load_base_address,debug_data_start));break;
        case 1:
          write32(debug_abstract,6,lh(S0,S0,ZERO));
          write32(debug_abstract,7,sh(S0,load_base_address,debug_data_start));break;
        case 2:
          write32(debug_abstract,6,lw(S0,S0,ZERO));
          write32(debug_abstract,7,sw(S0,load_base_address,debug_data_start));break;
        case 3:
          write32(debug_abstract,6,ld(S0,S0,ZERO));
          write32(debug_abstract,7,sd(S0,load_base_address,debug_data_start));break;
      }
      write32(debug_abstract,8, csrr(S0, CSR_DSCRATCH0)); // restore S0 again from dscratch
      (has_second_scratch)? write32(debug_abstract, 9, csrr(A0, CSR_DSCRATCH1)) : write32(debug_abstract, 9, nop()); // restore A0 again from dscratch1
      // write32(debug_abstract,9,nop());
      write32(debug_abstract,10,ebreak());
      cvm::log(cvm::HIGH, "Access Memory Read uCode update v2\n");
    }

  }
  else
  {
    abstractcs.cmderr = CMDERR_NOTSUP;
  }
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
      write32(program_buffer.data(), i, value);

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
      dmcontrol.hartsel = get_field(value, DM_DMCONTROL_HARTSELHI) << DM_DMCONTROL_HARTSELLO_LENGTH;
      dmcontrol.hartsel |= get_field(value, DM_DMCONTROL_HARTSELLO);
      dmcontrol.hartsel = size_t(dmcontrol.hartsel);
      for (const auto &[hart_id, hart] : harts)
      {
        if (hart_selected(hart_id))
        {
          cvm::log(cvm::FULL, "Inside the DMCONTROL Write Event - 002\n");
          if (get_field(value, DM_DMCONTROL_ACKHAVERESET))
          {
            hart_state[hart_id].havereset = false;
          }
          if (dmcontrol.haltreq && hart_available(hart_id))
          {
            hart->halt_request = hart->HR_REGULAR;
            cvm::log(cvm::HIGH, "halt hart: {:#x}\n", hart_id);
          }
          else
          {
            hart->halt_request = hart->HR_NONE;
          }
          if (dmcontrol.resumereq && hart_available(hart_id))
          {
            cvm::log(cvm::HIGH, "resume hart: {:#x}", hart_id);
            debug_rom_flags[hart_id] |= (1 << DEBUG_ROM_FLAG_RESUME);
            hart_state[hart_id].resumeack = false;
          }
          if (dmcontrol.hartreset && hart_available(hart_id))
          {
            hart->reset();
          }
        }
      }

      if (dmcontrol.ndmreset)
      {
        for (const auto &[hart_id, hart] : harts)
        {
          hart->reset();
        }
      }
    }
      return true;

    case DM_COMMAND:
      command = value;
      return perform_abstract_command();

      // case DM_HAWINDOWSEL:
      //   hawindowsel = value & ((1U<<field_width(hart_array_mask.size()))-1);
      //   return true;

      // case DM_HAWINDOW:
      //   {
      //     unsigned base = hawindowsel * 32;
      //     for (unsigned i = 0; i < 32; i++) {
      //       unsigned n = base + i;
      //       if (n < sim->get_cfg().nprocs()) {
      //         hart_array_mask[sim->get_cfg().hartids()[n]] = (value >> i) & 1;
      //       }
      //     }
      //   }
      //   return true;

    case DM_ABSTRACTCS:
      abstractcs.cmderr = (cmderr_t)(((uint32_t)(abstractcs.cmderr)) & (~(uint32_t)(get_field(value, DM_ABSTRACTCS_CMDERR))));
      return true;

    case DM_ABSTRACTAUTO:
      abstractauto.autoexecprogbuf = get_field(value,
                                               DM_ABSTRACTAUTO_AUTOEXECPROGBUF);
      abstractauto.autoexecdata = get_field(value,
                                            DM_ABSTRACTAUTO_AUTOEXECDATA);
      return true;
      // case DM_SBCS:
      //   sbcs.readonaddr = get_field(value, DM_SBCS_SBREADONADDR);
      //   sbcs.sbaccess = get_field(value, DM_SBCS_SBACCESS);
      //   sbcs.autoincrement = get_field(value, DM_SBCS_SBAUTOINCREMENT);
      //   sbcs.readondata = get_field(value, DM_SBCS_SBREADONDATA);
      //   sbcs.error &= ~get_field(value, DM_SBCS_SBERROR);
      //   return true;
      // case DM_SBADDRESS0:
      //   sbaddress[0] = value;
      //   if (sbcs.error == 0 && sbcs.readonaddr) {
      //     sb_read();
      //     sb_autoincrement();
      //   }
      //   return true;
      // case DM_SBADDRESS1:
      //   sbaddress[1] = value;
      //   return true;
      // case DM_SBADDRESS2:
      //   sbaddress[2] = value;
      //   return true;
      // case DM_SBADDRESS3:
      //   sbaddress[3] = value;
      //   return true;
      // case DM_SBDATA0:
      //   sbdata[0] = value;
      //   if (sbcs.error == 0) {
      //     sb_write();
      //     if (sbcs.error == 0) {
      //       sb_autoincrement();
      //     }
      //   }
      //   return true;
      // case DM_SBDATA1:
      //   sbdata[1] = value;
      //   return true;
      // case DM_SBDATA2:
      //   sbdata[2] = value;
      //   return true;
      // case DM_SBDATA3:
      //   sbdata[3] = value;
      //   return true;
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
      // case DM_DMCS2:
      //   if (config.support_haltgroups &&
      //       get_field(value, DM_DMCS2_HGWRITE) &&
      //       get_field(value, DM_DMCS2_GROUPTYPE) == 0) {
      //     selected_hart_state().haltgroup = get_field(value, DM_DMCS2_GROUP);
      //   }
      //   return true;
      // case DM_CUSTOM:
      //   for (unsigned i = 0; i < sizeof(hart_available_state) / sizeof(*hart_available_state); i++) {
      //     hart_available_state[i] = get_field(value, 1<<i);
      //   }
      //   return true;
    }
  }
  return false;
}

void debug_module_t::proc_reset(unsigned id)
{
  hart_state[id].havereset = true;
  hart_state[id].halted = false;
  hart_state[id].haltgroup = 0;
}

hart_debug_state_t &debug_module_t::selected_hart_state()
{
  return hart_state[selected_hart_id()];
}

size_t debug_module_t::selected_hart_id() const
{
  return dmcontrol.hartsel;
}
