#pragma once

#include "cvm/messenger.hpp"
CVM_MESSENGER_procedure_call(sysmod_eot, void (uint64_t, size_t, device::data_t& data_t)); // sysmod - eot interface
CVM_MESSENGER_procedure_call(sysmod_secure_mem, void (uint64_t& , uint64_t&)); // RPC to get secure region bounds
CVM_MESSENGER_procedure_call(sysmod_backdoor_write, void (transactor::write_t&)); // sysmod - snoop_gen interface
