// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "sysmod/device.h"
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
#include "interrupter.h"
#include "debugger.h"
#include "evt_trigger.h"
#include "uc_helper.h"
#include "io_coh_helper.h"
#include "ras_helper.h"
#include "dma.h"
#include <mem_manager.h>

// Define a core local  (trickbox) at the given address
// and for the given hart count. The size will be 48k bytes.
class trickbox : public device
{
public:

  /// Define a trickbox device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  trickbox(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc );

  // Destructor.
  virtual ~trickbox();

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
  /// No-op if address is outside the range of this trickbox or if
  /// address is not properly aligned.
  void read(const transactor::read_t& r, data_t& data);

  // Write to this trickbox.
  void write(const transactor::write_t& w);
  virtual void backdoor_write(uint64_t addr, size_t length, data_t& data, strb_t& strb) override;  
  virtual void tick(uint64_t advance) override
  {
    for (auto& d : subdevices_) {
      d->tick(advance);
    }
  }

  virtual void is_dut_reset_req(bool dut_reset_req,uint64_t clocks,uint64_t divisor) override 
  {
    cvm::log(cvm::HIGH,"Value of dut_reset_req in trickbox is : {} at clocks {} \n",dut_reset_req,clocks);
    for (auto& d : subdevices_) {
      d->is_dut_reset_req(dut_reset_req,clocks,divisor);
    }
  }
  
   
  virtual void jtag_tick(uint64_t advance) override
  {
    for (auto& d : subdevices_) {
      d->jtag_tick(advance);
    }
  }

  virtual void overlay_tick(uint64_t advance) override
  {
    for (auto& d : subdevices_) {
      d->overlay_tick(advance);
    }
  }

  /// Initialize memory with elf file.
  bool init_elf(const std::string& path);

  mem_manager m_;
  
 
private:
  uint64_t interrupter_base = 0x9000000;
  cvm::topology::loc_t axi_mst_loc_l;

  std::vector<std::unique_ptr<subdevice> > subdevices_;
  pcg32 rng;
};

