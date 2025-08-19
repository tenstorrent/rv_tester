#ifndef _RISCV_DEBUG_MODULE_H
#define _RISCV_DEBUG_MODULE_H

#include <memory>
#include <stdint.h>
#include <set>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"
#include "sysmod/trickbox/debugger.h"
#include "sysmod/sysmod_plusargs.h"
#include "processor.h"
#include "debug_defines.h"
#include "opcodes.h"
#include "encoding.h"
#include "decode.h"
#include "debug_rom.h"
#include "debug_rom_defines.h"

#define max_hartid 8 // Define the maximum number of harts in the system
#define halt_on_reset false
#define core_fuse_map (int[]) {0}  

typedef uint64_t reg_t;

typedef struct
{
  unsigned progbufsize;
  bool require_authentication;
  unsigned abstract_rti;
  bool support_hasel;
  bool support_abstract_csr_access;
  bool support_abstract_fpr_access;
  bool support_haltgroups;
  bool support_impebreak;
} debug_module_config_t;


typedef struct
{
  bool haltreq;
  bool resumereq;
  bool ackhavereset;
  bool hasel;
  unsigned hartsel;
  bool hartreset;
  bool dmactive;
  bool ndmreset;
} dmcontrol_t;

typedef struct
{
  bool ndmresetpending;
  bool impebreak;
  bool allhavereset;
  bool anyhavereset;
  bool allnonexistant;
  bool anynonexistant;
  bool allunavail;
  bool anyunavail;
  bool allrunning;
  bool anyrunning;
  bool allhalted;
  bool anyhalted;
  bool allresumeack;
  bool anyresumeack;
  bool authenticated;
  bool authbusy;
  bool cfgstrvalid;
  unsigned version;
} dmstatus_t;

typedef struct
{
  bool core7_halted;
  bool core6_halted;
  bool core5_halted;
  bool core4_halted;
  bool core3_halted;
  bool core2_halted;
  bool core1_halted;
  bool core0_halted;
} haltsum_t;

typedef enum cmderr
{
  CMDERR_NONE = 0,
  CMDERR_BUSY = 1,
  CMDERR_NOTSUP = 2,
  CMDERR_EXCEPTION = 3,
  CMDERR_HALTRESUME = 4,
  CMDERR_OTHER = 7
} cmderr_t;

typedef struct
{
  bool busy;
  unsigned datacount;
  unsigned progbufsize;
  cmderr_t cmderr;
} abstractcs_t;

typedef struct
{
  unsigned autoexecprogbuf;
  unsigned autoexecdata;
} abstractauto_t;

typedef struct
{
  unsigned version;
  bool readonaddr;
  unsigned sbaccess;
  bool autoincrement;
  bool readondata;
  unsigned error;
  unsigned asize;
  bool access128;
  bool access64;
  bool access32;
  bool access16;
  bool access8;
} sbcs_t;

typedef struct
{
  bool grouptype;
  unsigned dmexttrigger;
  unsigned group;
  bool hgwrite;
  bool hgselect;
} dmcs2_t;

typedef struct
{
  bool halted;
  bool resumeack;
  bool havereset;
  uint8_t haltgroup;
  uint8_t resumegroup;
} hart_debug_state_t;

class debug_module_t
{
public:
  debug_module_t(cvm::topology::loc_t, unsigned);

  bool load(reg_t addr, size_t len, uint8_t *bytes);
  bool store(reg_t addr, size_t len, const uint8_t *bytes);

  bool dmi_read(unsigned address, uint32_t *value);
  bool dmi_write(unsigned address, uint32_t value);
  
  void configure();
  // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
  // void run_test_idle();

  // Called when one of the attached harts was reset.
  void proc_reset(unsigned id);

  void process(const rv_tester_transactions::dm_model::dmi_req<> &dmi_req);
  void process(const rv_tester_transactions::dm_model::dmi_status<> &dmi_status);
  void process(const rv_tester_transactions::dm_model::dmi_resp<> &dmi_resp);

  void process(const rv_tester_transactions::dm_model::dm_load_cmd<> &dm_load_cmd);
  void process(const rv_tester_transactions::dm_model::dm_load_data<> &dm_load_data);
  void process(const rv_tester_transactions::dm_model::dm_store<> &dm_store);
  void process(const rv_tester_transactions::dm_model::dm_req<> &dm_req);

private:
  // cvm::file_logger log;
  // uint32_t num_harts_enabled;
  uint32_t sent_count = 0, resp_count =0;

  static const unsigned datasize = 12; //Number of data registers
  debug_module_config_t config = {16, false, 0, true, true, true, true, true};

  // Actual size of the program buffer, which is 1 word bigger than we let on
  // to implement the implicit ebreak at the end.
  unsigned program_buffer_bytes;
  //static const unsigned debug_data_start = 0x380;
  static const unsigned debug_data_start = 0x388;
  unsigned debug_progbuf_start;


  //static const unsigned debug_abstract_size = 10;
  static const unsigned debug_abstract_size = 16;
  unsigned debug_abstract_start;
  // R/W this through custom registers, to allow debuggers to test that
  // functionality.
  // unsigned custom_base;

  std::unordered_set<uint8_t> dm_regs_to_check = {0x11, 0x16, 0x04, 0x10, 0x17, 0x32, 0x40};
  uint8_t reg_addr_to_check;
  uint32_t req_expect;
  bool req_resp_check = false;

  size_t load_req_length;
  reg_t load_req_addr;
  uint8_t load_bytes_data[8] = {};
  uint64_t expected_load_data;
  bool mem_load_check = false;
  uint8_t load_req_id;
  loc_t loc;

  std::map<size_t, std::unique_ptr<processor_t>> harts;

  uint8_t debug_rom_whereto[4];
  uint8_t debug_abstract[debug_abstract_size * 4];
  std::vector<uint8_t> program_buffer;
  uint8_t dmdata[datasize * 4];

  std::vector<hart_debug_state_t> hart_state;
  uint8_t debug_rom_flags[1024];

  void write32(uint8_t *rom, unsigned int index, uint32_t value);
  uint32_t read32(uint8_t *rom, unsigned int index);

  dmcontrol_t dmcontrol;
  dmstatus_t dmstatus;
  dmcs2_t dmcs2;
  abstractcs_t abstractcs;
  abstractauto_t abstractauto;
  uint32_t command;
  uint16_t hawindowsel;
  std::vector<bool> hart_array_mask;

  // uint32_t challenge;
  // const uint32_t secret = 1;

  bool hart_selected(unsigned hartid) const;
  void reset(bool is_dm_only_reset=false);
  void init_debug_abstract_buffer();
  bool perform_abstract_command();
  bool has_second_scratch = true;
  uint8_t load_base_address = 10; //GPR to store DM base addr
  bool reflow_flags = false;

  //hart enum
  std::vector<uint32_t> hart_pid{};
  uint32_t hart_pid_mask = 0;
  uint32_t num_pid_harts =0;
  // bool abstract_command_completed;
  // unsigned rti_remaining;

  size_t selected_hart_id() const;
  hart_debug_state_t &selected_hart_state();

  bool hart_available_state[max_hartid];
  bool hart_available(unsigned hart_id) const;
};

#endif
