// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "subdevice.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "whisper_client.h"
#include "cosim/bridge/bridge_plusargs.h"

// Define a core local interruptor (interrupter) at the given address
// and for the given hart count. The size will be 48k bytes.
class interrupter : public subdevice
{
public:

  /// Define a interrupter device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);

  // Destructor.
  virtual ~interrupter();

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
  }

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this interrupter or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);

   void read_dev(uint64_t addr, size_t length,  data_t& data) override;
  // Write to this interrupter. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb) override;

  virtual void tick(uint64_t advance) override
  {
    timer_ += advance;
    timer_advance = advance;
    cvm::log(cvm::FULL, "[Trickbox] IMSIC timer tick {} advance interval {}\n", timer_, timer_advance);
    processDelayedRandomInterrupts();

  }
  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
  virtual void driveMSIInterrupt(unsigned t_data)
  {
    uint8_t interrupt_num = t_data & 0xfff;
    unsigned interrupt_file = (t_data>>12) & 0xf;
    unsigned interrupt_hart = (t_data>>16) & 0xfff;
    unsigned vs_id = (t_data>>28) & 0xfff;
    bool is_vgien_intr = false;
    bool exp_err_rsp = (interrupt_hart < FLAGS_num_harts) ? false : true;

    cvm::log(cvm::HIGH,"[Trickbox] IMSIC interrupt num: {} interrupt file: {} Interrupt hart:{} hypervisor/supervisor id : {}\n", static_cast<uint32_t>(interrupt_num), interrupt_file, interrupt_hart, vs_id);

    uint32_t addr1 = 0x900;
    if(interrupt_file == 0x0){
       addr1 = msi_m_file_addr + (interrupt_hart << 18);
    }else if(interrupt_file == 0x01){
       addr1 = msi_s_file_addr + (interrupt_hart << 18);
    }else if(interrupt_file == 0x02){
      cvm::log(cvm::MEDIUM,"[Trickbox] Driving VS interrupt for hart: {}, intr_num: {}\n", interrupt_hart, interrupt_num);
      uint64_t data_misa;
      uint64_t mask;
      uint64_t poke_mask;
      uint64_t read_mask;
      bool valid;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), interrupt_hart, 0x301, data_misa, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : MISA in drive_interrupt()\n", interrupt_hart);

      if ((data_misa >> 7) & 0x1) { // guest external interrupts based on hstatus.VGEIN
        is_vgien_intr = true;
        uint64_t data;
        uint64_t mask;
        uint64_t poke_mask;
        uint64_t read_mask;
        bool valid;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), interrupt_hart, 0x600, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : HSTATUS in drive_interrupt()\n", interrupt_hart);
        uint32_t vgein = (data >> 12) & 0x3F;

        // Get HGEIE value
        uint64_t hgeie_data;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), interrupt_hart, 0x607, hgeie_data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : HGEIE in drive_interrupt()\n", interrupt_hart);

        // 50% chance for single vs dual interrupt
        bool generate_dual_interrupt = (rng() % 2) == 0;

        if (!generate_dual_interrupt) {
          // Single interrupt case
          if ((rng() % 100) < 70) {
            // 70% chance to use VGEIN
            vs_id = vgein;
            intr_vs_id_vgein_++;
          } else {
            // 30% chance to use random VS ID != VGEIN
            do {
              vs_id = (rng() % 5) + 1; // Range [1,5]
            } while (vs_id == vgein);
            intr_vs_id_random_++;
          }

          // Drive single interrupt
          addr1 = msi_vs_file_addr + (vs_id << 12) + (interrupt_hart << 18);
          uint32_t length = 0x40;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
          for (uint8_t i = 0; i < 64; ++i) {
            data.push_back(0x0);
            strb.push_back(0x0);
          }
          for (uint8_t i = 0; i < 4; ++i) {
            uint8_t currentByte = static_cast<uint8_t>((interrupt_num >> (8 * i)) & 0xFF);
            data[i] = currentByte;
            strb[i] = 0x1;
          }
          cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length, data, strb});
        } else {
          // Dual interrupt case
          // First interrupt with VGEIN
          addr1 = msi_vs_file_addr + (vgein << 12) + (interrupt_hart << 18);
          uint32_t length = 0x40;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
          for (uint8_t i = 0; i < 64; ++i) {
            data.push_back(0x0);
            strb.push_back(0x0);
          }
          for (uint8_t i = 0; i < 4; ++i) {
            uint8_t currentByte = static_cast<uint8_t>((interrupt_num >> (8 * i)) & 0xFF);
            data[i] = currentByte;
            strb[i] = 0x1;
          }
          cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length, data, strb});

          // Second interrupt with different VS ID
          uint32_t second_vs_id;
          if ((rng() % 100) < 70) {
            // 70% chance to pick VS ID with HGEIE set
            do {
              second_vs_id = (rng() % 5) + 1; // Range [1,5]
            } while ((second_vs_id == vgein && (hgeie_data != (1ULL << vgein))) || ((hgeie_data & 0x3E) != 0 && !(hgeie_data & (1ULL << second_vs_id))));
          } else {
            // 30% chance to pick VS ID with HGEIE not set
            do {
              second_vs_id = (rng() % 5) + 1; // Range [1,5]
            } while ((second_vs_id == vgein && (hgeie_data != (0x3e & ~(1ULL << vgein)))) || ((hgeie_data & 0x3E) != 0x3E && (hgeie_data & (1ULL << second_vs_id))));
          }

          // Drive second interrupt
          addr1 = msi_vs_file_addr + (second_vs_id << 12) + (interrupt_hart << 18);
          cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length, data, strb});
          intr_vs_id_two_++;
        }
      } else {
        is_vgien_intr = false;
        vs_id = (rng() % 5) + 1; // Range [1,5]
        addr1 = msi_vs_file_addr+ (vs_id << 12) + (interrupt_hart << 18);
      }

    }else{
       cvm::log(cvm::ERROR, "Error: [Trickbox] Wrong IMSIC interrupt file specified\n");
    }
    uint32_t length1 = 0x40;
    // std::vector<uint8_t> data1 = {interrupt_num};
    //std::vector<uint8_t> data1 = {0xba,0xad,0xf0,0x12};
    // std::vector<bool> strb1 = {0,0,0,0,1,1,1,1};
    std::vector<uint8_t> data1;
    std::vector<bool> strb1;
    for (uint8_t i = 0; i < 64; ++i) {
      data1.push_back(0x0);
      strb1.push_back(0x0);
    }
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t currentByte = static_cast<uint8_t>((interrupt_num >> (8 * i)) & 0xFF);
      data1[i] = currentByte;
      strb1[i] = 0x1;
    }
    if (!is_vgien_intr) {
      cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1, exp_err_rsp});
    }

  }

  void reset() override {
    if(FLAGS_random_imsic_intr){
      cvm::log(cvm::MEDIUM, "[Trickbox] Enable random IMSIC MSIs {:#x}\n", FLAGS_random_imsic_intr);
      uint32_t rand_num =  (rng() %  2)+1;  //default delay
      if(FLAGS_imsic_intr_delay_min){
         rand_num = (rng() % ( FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
      }
      timer_ = 0;

      timer_rand_intr = timer_ + FLAGS_imsic_intr_start_delay +(rand_num*timer_advance);

    }

  }

  struct interrupt_t {
    unsigned hart;
    unsigned intr_select;
    unsigned intr_value;
  };

protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processDelayedRandomInterrupts()
  {

    //RANDOM imsic_intr
    if(FLAGS_random_imsic_intr && (intr_count <= (int)FLAGS_max_intr_count)){
      if(timer_ >= timer_rand_intr){
        unsigned intr_num = 1;
        unsigned intr_file = 0;
        unsigned intr_hart = 0;
        unsigned intr_vs_id = 0;
	unsigned disable_flags = FLAGS_disable_m_imsic_intr |( FLAGS_disable_s_imsic_intr <<1) |( FLAGS_disable_vs_imsic_intr <<2);
        if(disable_flags == 0x7)
	      cvm::log(cvm::ERROR, "Error:[Trickbox] Cant generate IMSIC interrupts when all interrupts are disabled \n");

	do{
        intr_file = (rng() % (3 )) ; //gen iter between 1 to max simul instr
	}while(((1<< intr_file)& disable_flags) != 0);

  intr_num =  (rng() % (FLAGS_imsic_intr_threshold ));
  if(!FLAGS_disable_vs_imsic_intr)
          intr_num = (rng() % (FLAGS_imsic_vs_intr_threshold )) ; //gen iter between 1 to max simul instr

	if(!FLAGS_disable_random_hart_imsic_intr)
          intr_hart = (rng() % (FLAGS_imsic_hart_threshold )) ; //gen iter between 1 to max simul instr
	if(!FLAGS_disable_vs_imsic_intr)
          intr_vs_id = (rng() % (FLAGS_imsic_vs_id_threshold )) ; //gen iter between 1 to max simul instr

        intr_num = intr_num |(intr_file<<12)|(intr_hart<<16)|(intr_vs_id<<28);
        cvm::log(cvm::HIGH, "[Trickbox] Driving imsic_intr {} interrupts in a cycle \n", intr_num);
        driveMSIInterrupt(intr_num);
        if(limit_interrupts){
          intr_count++;
        }
        uint32_t rand_num =  (rng() % ( FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
        timer_rand_intr = timer_ +(rand_num*timer_advance);
	      cvm::log(cvm::HIGH, "[Trickbox] Next random imsic_intr will be sent at  {}  \n", timer_rand_intr);
      }
    }

  }


  //Check plusarg usage
  void checkUsage();

private:
 //unsigned numMSIs_ = 6;
  cvm::topology::loc_t axi_mst_loc_l;
  //cvm::topology::loc_t intr_loc_;
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_           = 0;
  uint64_t timer_advance    = 200;
  uint64_t timer_rand_intr  = 500;
  uint64_t interrupter_base  = 0x9000000;
  uint32_t msi_m_file_addr  = 0x40000000;
  uint32_t msi_s_file_addr  = 0x44000000;
  uint32_t msi_vs_file_addr = 0x44000000;
  //IMSIC_ADDR_TARGET_M   = 32'h0100_0000,//32'h0800_0000;
  // IMSIC_ADDR_TARGET_S   = 32'h0180_0000,//32'h0A00_0000;
  int      intr_count = 0;
  int      limit_interrupts = 0;
  const unsigned hart_count_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;

  uint32_t intr_vs_id_vgein_ = 0;
  uint32_t intr_vs_id_random_ = 0;
  uint32_t intr_vs_id_two_ = 0;
};
