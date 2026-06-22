#pragma once

#include "cvm/messenger.hpp"
#include "src/sysmod/device.h"
CVM_MESSENGER_procedure_call(sysmod_eot, void(uint64_t, size_t, device::data_t& data_t));                                         // sysmod - eot interface
CVM_MESSENGER_procedure_call(sysmod_secure_mem, void(uint64_t&, uint64_t&));                                                      // RPC to get secure region bounds
CVM_MESSENGER_procedure_call(sysmod_backdoor_write, void(transactor::write_t&));                                                  // sysmod - snoop_gen interface
CVM_MESSENGER_procedure_call(sysmod_backdoor_write_boot, void(uint64_t, uint32_t, const device::data_t&, const device::strb_t&)); // sysmod_core_agent - sysmod interface
CVM_MESSENGER_procedure_call(sysmod_get_boot_addr, uint64_t());                                                                   // sysmod_core_agent - sysmod interface
CVM_MESSENGER_procedure_call(sysmod_block_terminate, void());                                                                     // defer the sim-end callback until matching unblock_terminate
CVM_MESSENGER_procedure_call(sysmod_unblock_terminate, void());                                                                   // pair with block_terminate; pushes sim-end callback when count returns to 0