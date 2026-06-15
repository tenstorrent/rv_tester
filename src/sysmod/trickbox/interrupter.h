// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"

// Directed MSI request signaled from trickbox to external_interrupt_sequence.
// Fields packed in data: hart[27:16], file[15:12], vs_id[33:28], disable_vs_rand[40]
struct directed_msi_request_t {
  uint64_t intr_num;
  uint64_t packed_data;
};

// Trickbox I/O handler for interrupt-related memory-mapped registers:
//   - Base+0x0000 : directed MSI write (forwarded to external_interrupt_sequence)
//   - Base+0x2000 : mnscratch read/write
//   - Base+0x4040 : interrupt enable flag (consumed by NMI sequence via RPC)
//   - Base+0x5000 : NMI deassert
class interrupter : public subdevice
{
public:

  interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  virtual ~interrupter();

  void configure() override;

  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
  }

  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);

  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb) override;

  virtual void tick(uint64_t) override { }

  CVM_MESSENGER_procedure_call(intr_enable_read_RPC, bool (bool&));

protected:
  void checkUsage();

private:
  uint64_t interrupter_base = 0x9000000;
  const unsigned hart_count_;
  bool intr_enable_flag_ = false;
  cvm::topology::loc_t loc_;
};
