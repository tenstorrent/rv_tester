// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>
#include "common/pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <mem_manager.h>
#include "transactor.h"
#include "src/transactors/axi_sw/axi.h"
#include "whisper_client.h"
#include "src/sysmod/trickbox/io_coh_helper.h"

class dma : public subdevice {
public:
  dma(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, mem_manager& m_, io_coh_helper* io_coh_helper_ptr = nullptr);
  // Destructor.
  virtual ~dma();

  void configure() override;

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }

  cvm::messenger::task<void> call_io_coh_helper_write(uint64_t addr, std::vector<uint8_t> ext_data_vec, uint64_t size, bool write_in_flight);
  cvm::messenger::task<void> call_snoop_gen_read(uint64_t& addr, std::vector<uint8_t>& ext_read_data_vec, uint8_t size, bool read_in_flight);
  // cvm::messenger::task<void> call_io_coh_helper_read(uint64_t addr, uint64_t size);

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this dma or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);
  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  // Write to this dma. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;
  void reset() override {
  }

  uint64_t convertToUInt64(const std::vector<uint8_t>& vec) {
    if (vec.size() != 8) {
      throw std::invalid_argument("Vector must have exactly 8 elements.");
    }

    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i) {
      result |= static_cast<uint64_t>(vec[i]) << (8 * i);
    }

    return result;
  }

  void gen_data_strb(uint64_t addr, data_t& wdata, std::vector<bool>& strb);

  typedef struct dma_txn {
    uint64_t addr;
    uint64_t data[8];
    uint64_t size;
    uint8_t cmd;
    uint8_t status;
    bool virt;
    bool in_flight = false;
  } dma_txn;

protected:
  void dma_write(uint64_t addr, uint64_t data);
  void dma_read(uint64_t addr);
  void overlay_write(uint64_t addr, uint8_t map_key);
  cvm::messenger::task<axi::r_t> overlay_read(uint64_t addr, uint8_t map_key);
  cvm::messenger::task<void> blocking_read(uint64_t addr);
  cvm::messenger::task<void> blocking_write(uint64_t addr);
  cvm::messenger::task<void> handle_dma_read_request(uint8_t map_key);

private:
  cvm::topology::loc_t iommu_tr_req_loc_;
  cvm::messenger::pool<axi::b_t>::channel_info wresp_channel;
  cvm::messenger::pool<axi::r_t>::channel_info rresp_channel;

  uint64_t dma_base_addr_ = 0x9090000;
  uint64_t dma_addr_offset_ = 0x0;
  uint64_t dma_data_offset_ = 0x200;
  uint64_t dma_size_offset_ = 0x1200;
  uint64_t dma_cmd_offset_ = 0x1240;
  uint64_t dma_virt_offset_ = 0x1280; // remove later
  uint64_t dma_status_offset_ = 0x12c0;

  uint8_t curr_dma_cmd_ = 0; // 1 for read, 2 for write -> enhance for COPY later
  uint64_t curr_dma_data_ = 0;
  uint8_t curr_dma_size_ = 0;
  uint64_t curr_dma_addr_ = 0;
  uint64_t curr_dma_virt_ = 0; // 0 for bare mode , 1 for virtual mode
  uint64_t curr_dma_status_ = 0;

  bool dma_req_in_flight_ = false;
  bool use_overlay_write_ = true; // Set to true to use overlay_write, false for blocking_write

  std::map<uint8_t, dma_txn> dma_txn_map_;

  std::vector<uint8_t> dma_write_data_vec_;
  std::vector<bool> dma_write_strb_vec_;

  uint64_t dma_write_addr_ = 0;
  uint8_t dma_write_size_ = 0;
  uint8_t dma_map_key_ = 0;
  uint64_t dma_read_addr_ = 0;
  uint8_t dma_read_size_ = 0;
  uint64_t num_writes = 0;

  uint8_t axi_id = 0;

  bool write_in_flight = false;
  bool read_in_flight = false;
  bool burst_in_flight = false;
  bool poke_valid_ = false;

  axi::r_t resp_;

  // Reference to io_coh_helper instance
  io_coh_helper* io_coh_helper_ptr_;
  // Reference to snoop_gen_sequence instance
  mem_manager m_;

  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};
