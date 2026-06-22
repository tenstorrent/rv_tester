#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "src/sysmod/trickbox/dma.h"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"
#include "rv_tester_plusargs.h"

dma::dma(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager& m_, io_coh_helper* io_coh_helper_ptr)
    : subdevice(tag, addr, 0x2000, loc), io_coh_helper_ptr_(io_coh_helper_ptr), m_(m_) {
  iommu_tr_req_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST", 0);
  cvm::log(cvm::HIGH, "[dma] iommu_tr_req_loc_ : {} \n", iommu_tr_req_loc_);
}

void dma::configure() {
  subdevice::configure();
  wresp_channel = cvm::registry::messenger.channel<axi::b_t>(iommu_tr_req_loc_);
  rresp_channel = cvm::registry::messenger.channel<axi::r_t>(iommu_tr_req_loc_);
}

dma::~dma() {
  // Destructor implementation
}

void dma::read_dev(uint64_t addr, size_t length, data_t& data) {

  if (not has_addr(addr)) {
    cvm::log(cvm::HIGH, "[dma] Discarding read request at dma device since tag {} is not matching for address {:#x} \n", tag(), addr);
    return;
  }
  cvm::log(cvm::HIGH, "[dma] read address: {:#x} \n", addr);
  if (addr >= (dma_base_addr_ + dma_status_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_status_offset_));
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t status_data = (uint64_t)dma_txn_map_[dma_map_key_].status;
      serializeInt(status_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Status is not found for DMA status buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if ((addr >= dma_base_addr_ + dma_addr_offset_) && (addr < dma_base_addr_ + dma_data_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_addr_offset_)) / 8;
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t addr_data = (uint64_t)dma_txn_map_[dma_map_key_].addr;
      serializeInt(addr_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Address is not found for DMA address buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if ((addr >= dma_base_addr_ + dma_data_offset_) && (addr < dma_base_addr_ + dma_size_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_data_offset_)) / 64;
    cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t data_data = (uint64_t)dma_txn_map_[dma_map_key_].data[0];
      serializeInt(data_data, length, data);
      cvm::log(cvm::HIGH, "[dma] data_data : {:#x} \n", data_data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Data not found for DMA data buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if ((addr >= dma_base_addr_ + dma_size_offset_) && (addr < dma_base_addr_ + dma_cmd_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_size_offset_));
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t size_data = (uint64_t)dma_txn_map_[dma_map_key_].size;
      serializeInt(size_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Size not found for DMA size buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if ((addr >= dma_base_addr_ + dma_cmd_offset_) && (addr < dma_base_addr_ + dma_virt_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_cmd_offset_));
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t cmd_data = (uint64_t)dma_txn_map_[dma_map_key_].cmd;
      serializeInt(cmd_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Command not found for DMA command buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if ((addr >= dma_base_addr_ + dma_virt_offset_) && (addr < dma_base_addr_ + dma_status_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_virt_offset_));
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t virt_data = (uint64_t)dma_txn_map_[dma_map_key_].virt;
      serializeInt(virt_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Virt not found for DMA virt buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }
  if (addr >= dma_base_addr_ + dma_status_offset_) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_status_offset_));
    if ((dma_txn_map_.find(dma_map_key_) != dma_txn_map_.end())) {
      uint64_t status_data = (uint64_t)dma_txn_map_[dma_map_key_].status;
      serializeInt(status_data, length, data);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Status not found for DMA status buffer read request.  DMA trickbox address {:#x} \n", addr);
    }
  }

  return;
}

void dma::overlay_write(uint64_t addr, uint8_t map_key) {

  cvm::log(cvm::HIGH, "[dma] axi write addr= {:#X} for map_key {}   \n", addr, map_key);
  uint64_t waddr = addr;

  auto* l = +[](uint64_t waddr, dma* dev) -> cvm::messenger::task<void> {
    co_await dev->blocking_write(waddr);
  };

  cvm::registry::messenger.fork(l, waddr, this);
  dma_txn_map_[map_key].in_flight = false;
  dma_txn_map_[map_key].status = 2;
}

cvm::messenger::task<void> dma::blocking_write(uint64_t addr) {
  bool valid;
  axi::a_t aw_txn;
  aw_txn.w = true;
  aw_txn.id = axi_id++;
  aw_txn.addr = dma_write_addr_;
  aw_txn.len = 0;
  aw_txn.size = log2(dma_write_size_); //3;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock = 0;
  aw_txn.cache = axi::cache_mem_attr_t(0);
  //  Set PROT based on 55th bit
  aw_txn.prot = (dma_write_addr_ & (1ULL << 55)) ? 0 : 2;
  aw_txn.qos = 0;
  aw_txn.region = 0;
  aw_txn.atop = 0;
  aw_txn.user = 8;
  aw_txn.allow_decerr_resp = (aw_txn.addr & 0x3) || (aw_txn.size == 0 || aw_txn.size == 1) || ((aw_txn.addr & 0x7) == 4 && aw_txn.size >= 3);

  cvm::log(cvm::HIGH, "[dma] In Blocking write function for addr: {:#x}   \n", dma_write_addr_);

  cvm::registry::messenger.signal(iommu_tr_req_loc_, aw_txn);
  axi::w_t w_txn;
  std::vector<uint8_t> data_vec = dma_write_data_vec_;
  std::vector<bool> strb_vec = dma_write_strb_vec_;
  cvm::log(cvm::HIGH, "[dma] data_vec size : {} \n", data_vec.size());
  cvm::log(cvm::HIGH, "[dma] strb_vec size : {} \n", strb_vec.size());

  for (uint8_t i = 0; i < dma_write_size_; ++i) {
    w_txn.data.push_back(data_vec[i]);
    w_txn.strb.push_back(strb_vec[i]);
  }
  cvm::log(cvm::HIGH, "[dma] w_txn data size : {} \n", w_txn.data.size());
  cvm::log(cvm::HIGH, "[dma] w_txn strb size : {} \n", w_txn.strb.size());

  w_txn.last = 1;
  uint32_t wresp_id = aw_txn.id;
  cvm::log(cvm::HIGH, "[dma] Wresp_id : {:#x}   \n", wresp_id);
  cvm::registry::messenger.signal(iommu_tr_req_loc_, w_txn);

  write_in_flight = true;
  co_await cvm::registry::messenger.wait<axi::b_t>(wresp_channel, [&wresp_id](const axi::b_t& wresp) { return wresp.id == wresp_id; });

  write_in_flight = false;

  dma_write_strb_vec_.clear();
  dma_write_data_vec_.clear();

  //Poke same data to whisper memory
  for (uint8_t i = 0; i < dma_write_size_; ++i) {
    cvm::log(cvm::HIGH, "[dma] Backdoor whisper poke addr{:#x} poke_data {:#x} \n", (addr + i), data_vec[i]);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', addr + i, 1, data_vec[i], false, false, valid) || !valid) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    cvm::log(cvm::HIGH, "[dma] backdoor whisper poke  Successful for addr{:#x} poke_data {:#x} \n", addr + i, data_vec[i]);
  }
  num_writes++;
  co_return;
}

cvm::messenger::task<axi::r_t> dma::overlay_read(uint64_t addr, uint8_t map_key) {
  cvm::log(cvm::FULL, "[dma] axi read addr= {:#X}   \n", addr);
  dma_read_addr_ = addr;
  dma_map_key_ = map_key;
  dma_read_size_ = dma_txn_map_[map_key].size;

  uint64_t raddr = addr;

  // Execute the blocking read and return the response
  co_await blocking_read(raddr);

  dma_txn_map_[map_key].in_flight = false;
  dma_txn_map_[map_key].status = 2;

  co_return resp_;
}

cvm::messenger::task<void> dma::handle_dma_read_request(uint8_t map_key) {

  auto read_resp = co_await overlay_read(dma_read_addr_, map_key);
  // We'll get a 512 bit payload from here
  // Need to pick the bytes of interest from here

  for (unsigned i = 0; i < read_resp.data.size(); i++) {
    cvm::log(cvm::HIGH, "[dma] read_resp data [{}] : {:#x} \n", i, read_resp.data[i]);
  }

  uint32_t data_offset = dma_read_addr_ & 0x3f; // lower 6 bits

  uint64_t data_value = 0;
  cvm::log(cvm::HIGH, "[dma] read map key: {} \n", map_key);

  switch (dma_txn_map_[map_key].size) {
  case 1:
    dma_txn_map_[map_key].data[0] = uint64_t(read_resp.data[data_offset]);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + map_key * 8), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    break;
  case 2:
    for (int i = 0; i < 2; i++) {
      data_value |= uint64_t(read_resp.data[data_offset + i]) << (i * 8);
    }
    dma_txn_map_[map_key].data[0] = data_value;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + map_key * 8), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    break;
  case 4:
    for (int i = 0; i < 4; i++) {
      data_value |= uint64_t(read_resp.data[data_offset + i]) << (i * 8);
    }
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + map_key * 8), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    dma_txn_map_[map_key].data[0] = data_value;
    break;
  case 8:
    for (int i = 0; i < 8; i++) {
      data_value |= uint64_t(read_resp.data[data_offset + i]) << (i * 8);
    }
    cvm::log(cvm::HIGH, "[dma] data_value : {:#x} \n", data_value);
    dma_txn_map_[map_key].data[0] = data_value;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + map_key * 8), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    break;
  case 16:
    for (int buffer_id = 1; buffer_id >= 0; buffer_id--) {
      data_value = 0; // Reset data_value for each buffer
      for (int i = 0; i < 8; i++) {
        data_value |= uint64_t(read_resp.data[data_offset + (buffer_id + 1) * 8 - i - 1]) << (i * 8);
        // extract the data from the data_vec in little endian order
      }
      dma_txn_map_[map_key].data[buffer_id] = data_value;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + (map_key * 8) + (buffer_id * 8)), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
        co_return;
      }
    }
    break;
  case 32:
    for (int buffer_id = 3; buffer_id >= 0; buffer_id--) {
      data_value = 0; // Reset data_value for each buffer
      for (int i = 0; i < 8; i++) {
        data_value |= uint64_t(read_resp.data[data_offset + (buffer_id + 1) * 8 - i - 1]) << (i * 8);
        // extract the data from the data_vec in little endian order
      }
      dma_txn_map_[map_key].data[buffer_id] = data_value;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + (map_key * 8) + (buffer_id * 8)), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
        co_return;
      }
    }
    break;
  case 64:
    for (int buffer_id = 7; buffer_id >= 0; buffer_id--) {
      data_value = 0; // Reset data_value for each buffer
      for (int i = 0; i < 8; i++) {
        data_value |= uint64_t(read_resp.data[data_offset + (buffer_id + 1) * 8 - i - 1]) << (i * 8);
        // extract the data from the data_vec in little endian order
      }
      dma_txn_map_[map_key].data[buffer_id] = data_value;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (dma_base_addr_ + dma_addr_offset_ + (map_key * 8) + (buffer_id * 8)), 8, data_value, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
        co_return;
      }
    }
    break;
  default:
    cvm::log(cvm::ERROR, "[dma] ERROR: Invalid size {:#x} for DMA read request.  DMA trickbox address {:#x} \n", dma_txn_map_[map_key].size, dma_read_addr_);
    co_return;
  }
  dma_txn_map_[map_key].in_flight = false;
  dma_txn_map_[map_key].status = 2; // setting to COMPLETED

  co_return;
}

cvm::messenger::task<void> dma::blocking_read(uint64_t addr) {

  axi::a_t ar_txn;
  uint32_t id;
  ar_txn.w = false;
  ar_txn.addr = addr;
  ar_txn.size = log2(dma_read_size_);
  ar_txn.len = 0;
  ar_txn.id = axi_id++;
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock = 0;
  ar_txn.cache = axi::cache_mem_attr_t(0);
  ar_txn.prot = 2;
  ar_txn.qos = 0;
  ar_txn.region = 0;
  ar_txn.atop = 0;
  ar_txn.user = 0;
  ar_txn.allow_decerr_resp = (ar_txn.addr & 0x3) || (ar_txn.size == 0 || ar_txn.size == 1) || ((ar_txn.addr & 0x7) == 4 && ar_txn.size >= 3);

  cvm::log(cvm::HIGH, "[dma] In Blocking read function for addr: {:#x}   \n", dma_read_addr_);

  id = ar_txn.id;
  cvm::log(cvm::HIGH, "[dma] ar_txn id : {:#x} \n", id);
  cvm::registry::messenger.signal(iommu_tr_req_loc_, ar_txn);
  read_in_flight = true;

  axi::r_t rresp = co_await cvm::registry::messenger.wait<axi::r_t>(
      rresp_channel,
      [&id](const axi::r_t& r) { return r.id == id; });

  resp_ = rresp;
  read_in_flight = false;
  co_return;
}

void dma::write(uint64_t addr, size_t, const data_t& data, const strb_t&) {
  if (not has_addr(addr)) {
    cvm::log(cvm::HIGH, "[dma] Discarding write request to dma device since tag {} is not matching for address {:#x} \n", tag(), addr);
    return;
  }
  cvm::log(cvm::HIGH, "[dma] write addr {:#x}  \n", addr);
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[dma] write data {:#x} \n", t_data);
  // Poke the data into whisper memory
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', addr, 8, t_data, false, false, poke_valid_) || !poke_valid_) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
    return;
  }

  // DMA Address Buffer Handling
  if ((addr >= dma_base_addr_ + dma_addr_offset_) && (addr < dma_base_addr_ + dma_data_offset_)) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_addr_offset_)) / 8;
    if ((addr - (dma_base_addr_ + dma_addr_offset_)) % 8 != 0) {
      cvm::log(cvm::ERROR, "[dma] ERROR: Address {:#x} is not aligned to 8 bytes for writing into DMA trickbox address buffers  \n", addr);
      return;
    }
    if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
      dma_txn_map_[dma_map_key_].addr = t_data;
      // Configured Address
      cvm::log(cvm::HIGH, "[dma] address {:#x} written to DMA trickbox address buffer : {} \n", t_data, dma_map_key_);
      cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Address being changed for a DMA transaction that's already in flight.  DMA trickbox address {:#x} \n", addr);
    }
  }

  // DMA Data Buffer Handling
  else if ((addr >= (dma_base_addr_ + dma_data_offset_)) && (addr < (dma_base_addr_ + dma_size_offset_))) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_data_offset_)) / 64;
    // if((addr - (dma_base_addr_ + dma_data_offset_))%64 != 0){
    //   cvm::log(cvm::ERROR, "[dma] ERROR: Address {:#x} is not aligned to 64 bytes for writing into DMA trickbox data buffers \n",addr);
    //   return;
    // }
    if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
      uint8_t byte_index = (addr - (dma_base_addr_ + dma_data_offset_)) / 8;
      dma_txn_map_[dma_map_key_].data[byte_index] = t_data;
      cvm::log(cvm::HIGH, "[dma] write data {:#x} to DMA trickbox data buffer : {} \n", t_data, dma_map_key_);
      cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Data is being changed for a DMA data buffer whose txn is already in flight.  DMA trickbox address {:#x} \n", addr);
    }
  }

  // DMA Size Buffer Handling
  else if ((addr >= (dma_base_addr_ + dma_size_offset_)) && (addr < (dma_base_addr_ + dma_cmd_offset_))) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_size_offset_));
    if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
      dma_txn_map_[dma_map_key_].size = t_data;
      cvm::log(cvm::HIGH, "[dma] size {:#x} written to DMA trickbox size buffer : {} \n", t_data, dma_map_key_);
      cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Size is being changed for a DMA size buffer whose txn is already in flight.  DMA trickbox address {:#x} \n", addr);
    }
  }

  // DMA Command Buffer Handling
  else if (addr >= (dma_base_addr_ + dma_cmd_offset_) && (addr < (dma_base_addr_ + dma_virt_offset_))) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_cmd_offset_));
    if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
      cvm::log(cvm::HIGH, "[dma] command {:#x} written to DMA trickbox command buffer : {} \n", t_data, dma_map_key_);
      if (t_data == 1) {
        dma_txn_map_[dma_map_key_].cmd = 1;
      } else if (t_data == 2) {
        dma_txn_map_[dma_map_key_].cmd = 2;
      } else {
        cvm::log(cvm::ERROR, "[dma] ERROR: Invalid command {:#x} for DMA command buffer.  DMA trickbox address {:#x} \n", t_data, addr);
      }
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Command is being changed for a DMA command buffer whose txn is already in flight.  DMA trickbox address {:#x} \n", addr);
    }
  }

  // DMA Virt Buffer Handling
  else if (addr >= (dma_base_addr_ + dma_virt_offset_) && (addr < (dma_base_addr_ + dma_status_offset_))) {
    dma_map_key_ = (addr - (dma_base_addr_ + dma_virt_offset_));
    if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
      dma_txn_map_[dma_map_key_].virt = (t_data == 1);
      cvm::log(cvm::HIGH, "[dma] virt bit {:#x} written to DMA trickbox virt buffer : {} \n", t_data, dma_map_key_);
      cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
    } else {
      cvm::log(cvm::ERROR, "[dma] ERROR: Virt bit is being changed for a DMA virt buffer whose txn is already in flight.  DMA trickbox address {:#x} \n", addr);
    }
  }

  // DMA Status Buffer Handling
  else if (addr >= (dma_base_addr_ + dma_status_offset_)) {
    if ((addr - (dma_base_addr_ + dma_status_offset_)) > 64) {
      cvm::log(cvm::ERROR, "[dma] ERROR: Address {:#x} for writing into DMA status buffers is out of bounds\n", addr);
      cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} \n", dma_map_key_);
      return;
    } else {
      dma_map_key_ = (addr - (dma_base_addr_ + dma_status_offset_));
      if ((dma_txn_map_.find(dma_map_key_) == dma_txn_map_.end()) || (dma_txn_map_[dma_map_key_].in_flight == false)) {
        dma_txn_map_[dma_map_key_].status = t_data;
        cvm::log(cvm::HIGH, "[dma] status {:#x} written to DMA trickbox status buffer : {} \n", t_data, dma_map_key_);
      } else {
        cvm::log(cvm::ERROR, "[dma] ERROR: Status is being changed for a DMA status buffer whose txn is already in flight.  DMA trickbox address {:#x} \n", addr);
      }
    }

    // -------------------------------- dma read request handling --------------------------------

    if (dma_txn_map_[dma_map_key_].status == 1) {
      cvm::log(cvm::HIGH, "[dma] DMA status is set to 1, DMA request will take flight \n");
      dma_txn_map_[dma_map_key_].in_flight = true;
      std::vector<uint8_t> data_vec;
      data_vec = {}; // need to initialise the vector

      if (dma_txn_map_[dma_map_key_].cmd == 1) {
        // Signal a transactor read request
        cvm::log(cvm::HIGH, "[dma] DMA read request signaled to transactor \n");

        dma_txn_map_[dma_map_key_].in_flight = true;
        dma_read_addr_ = dma_txn_map_[dma_map_key_].addr;

        cvm::log(cvm::HIGH, "[dma] dma_read size : {} \n", dma_txn_map_[dma_map_key_].size);

        // Use overlay_read function to read the data from the transactor
        // overlay_read(dma_txn_map_[dma_map_key_].addr, dma_map_key_);

        cvm::log(cvm::FULL, "[dma] axi read addr= {:#X}   \n", addr);
        dma_read_addr_ = dma_txn_map_[dma_map_key_].addr;
        dma_read_size_ = dma_txn_map_[dma_map_key_].size;

        // Fork the async read request handling
        auto* read_handler = +[](uint8_t map_key, dma* dev) -> cvm::messenger::task<void> {
          co_await dev->handle_dma_read_request(map_key);
        };
        cvm::registry::messenger.fork(read_handler, dma_map_key_, this);

      }

      // -------------------------------- dma write request handling --------------------------------

      else if (dma_txn_map_[dma_map_key_].cmd == 2) {
        // Signal a transactor write request
        cvm::log(cvm::HIGH, "[dma] DMA write request signaled to transactor \n");
        // TODO: Call io_coh_helper's blocking_write task
        uint64_t data_value; // Declare data_value before switch statement
        dma_write_data_vec_.clear();
        dma_write_strb_vec_.clear();
        cvm::log(cvm::HIGH, "[dma] dma_map_key_ : {} , DMA Size : {} \n", dma_map_key_, dma_txn_map_[dma_map_key_].size);
        switch (dma_txn_map_[dma_map_key_].size) {
        case 1:
          dma_write_data_vec_.push_back(static_cast<uint8_t>(dma_txn_map_[dma_map_key_].data[0]));
          dma_write_strb_vec_.push_back(true);
          break;

        case 2:
          data_value = dma_txn_map_[dma_map_key_].data[0];
          for (int i = 0; i < 2; i++) {
            dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
            data_value >>= 8;
            dma_write_strb_vec_.push_back(true);
          }
          break;

        case 4:
          data_value = dma_txn_map_[dma_map_key_].data[0];
          for (int i = 0; i < 4; i++) {
            dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
            dma_write_strb_vec_.push_back(true);
            data_value >>= 8;
          }
          break;

        case 8:
          data_value = dma_txn_map_[dma_map_key_].data[0];
          cvm::log(cvm::HIGH, "[dma] data_value : {} \n", data_value);
          for (int i = 0; i < 8; i++) {
            dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
            dma_write_strb_vec_.push_back(true);
            data_value >>= 8;
            // Extract all 8 bytes from the first 64-bit data value (little-endian)
          }
          break;

        case 16:

          for (int buffer_id = 1; buffer_id >= 0; buffer_id--) {
            data_value = dma_txn_map_[dma_map_key_].data[buffer_id];
            for (int i = 0; i < 8; i++) {
              dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
              dma_write_strb_vec_.push_back(true);
              data_value >>= 8;
            }
          }
          break;

        case 32:
          for (int buffer_id = 3; buffer_id >= 0; buffer_id--) {
            data_value = dma_txn_map_[dma_map_key_].data[buffer_id];
            for (int i = 0; i < 8; i++) {
              dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
              dma_write_strb_vec_.push_back(true);
              data_value >>= 8;
            }
          }
          break;

        case 64:
          for (int buffer_id = 7; buffer_id >= 0; buffer_id--) {
            data_value = dma_txn_map_[dma_map_key_].data[buffer_id];
            for (int i = 0; i < 8; i++) {
              dma_write_data_vec_.push_back(static_cast<uint8_t>(data_value & 0xFF));
              dma_write_strb_vec_.push_back(true);
              data_value >>= 8;
            }
          }
          break;

        default:
          cvm::log(cvm::ERROR, "[dma] ERROR: Invalid size {:#x} for DMA write request.  DMA trickbox address {:#x} \n", dma_txn_map_[dma_map_key_].size, addr);
          return;
        }

        dma_txn_map_[dma_map_key_].in_flight = true;
        dma_write_addr_ = dma_txn_map_[dma_map_key_].addr;
        dma_write_size_ = dma_txn_map_[dma_map_key_].size;
        cvm::log(cvm::HIGH, "[dma] dma_write_addr_ : {}, dma_write_size_ : {} \n", dma_write_addr_, dma_write_size_);
        overlay_write(dma_txn_map_[dma_map_key_].addr, dma_map_key_);
      }
    }
  }
}
