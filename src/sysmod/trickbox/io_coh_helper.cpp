// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "trickbox/io_coh_helper.h"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"
#include "rv_tester_plusargs.h"
#include "device_address_map/device_address_map.h"

DEFINE_bool(debug_io_coh_helper, false, "Enable internal uc helper debug logging");

bool io_coh_helper::is_mmr_window(uint64_t addr) {
  constexpr unsigned kMmrAddrLsb = 27;
  uint64_t mmr_base = device_address_map_mmr_base_addr();
  if (mmr_base == 0)
    mmr_base = 0x40000000ULL;
  return (addr >> kMmrAddrLsb) == (mmr_base >> kMmrAddrLsb);
}

cvm::topology::loc_t io_coh_helper::mst_for_addr(uint64_t addr) const {
  return is_mmr_window(addr) ? axi_loc_mmr_ : axi_loc_ioc_;
}

cvm::messenger::pool<axi::b_t>::channel_info io_coh_helper::b_channel_for(cvm::topology::loc_t loc) const {
  return loc == axi_loc_mmr_ ? b_channel_mmr_ : b_channel_ioc_;
}

cvm::messenger::pool<axi::r_t>::channel_info io_coh_helper::r_channel_for(cvm::topology::loc_t loc) const {
  return loc == axi_loc_mmr_ ? r_channel_mmr_ : r_channel_ioc_;
}

io_coh_helper::io_coh_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager& m_)
    : subdevice(tag, addr, 0x1000, loc), m_(m_) {
  rng.seed(FLAGS_seed);
  io_coh_helper_base = addr;
  auto plat = cvm::topology::get_from_type("PLATFORM", 0);
  auto def = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  auto mmr_attr = cvm::topology::attr(plat, "IO_COH_MMR_MST");
  auto ioc_attr = cvm::topology::attr(plat, "IO_COH_IOC_MST");
  axi_loc_mmr_ = mmr_attr.first ? cvm::topology::loc_t(mmr_attr.second) : def;
  axi_loc_ioc_ = ioc_attr.first ? cvm::topology::loc_t(ioc_attr.second) : axi_loc_mmr_;
  reset();
  checkUsage();
}

void io_coh_helper::configure() {
  subdevice::configure();
  b_channel_mmr_ = cvm::registry::messenger.channel<axi::b_t>(axi_loc_mmr_);
  b_channel_ioc_ = cvm::registry::messenger.channel<axi::b_t>(axi_loc_ioc_);
  r_channel_mmr_ = cvm::registry::messenger.channel<axi::r_t>(axi_loc_mmr_);
  r_channel_ioc_ = cvm::registry::messenger.channel<axi::r_t>(axi_loc_ioc_);
}

cvm::messenger::task<void>
io_coh_helper::read(uint64_t addr, size_t length, data_t& data) {
  mem::datum_t m_data = 0;
  uint32_t word = (uint32_t)m_data;
  cvm::log(cvm::HIGH, "[io_coh_helper] COROUTINE read read addr {:#x} data {:#x} \n", addr, word);
  serializeInt(word, length, data);

  co_return;
}

void io_coh_helper::read_dev(uint64_t addr, size_t length, data_t& data) {

  if (not has_addr(addr)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Descarding read request at io_coh_helper since tag {} is not matching \n", tag());
    return;
  }
  cvm::log(cvm::HIGH, "[io_coh_helper] read address: {:#x} \n", addr);
  if (addr == (io_coh_helper_base + 0x500)) {
    if (tx_type == 1) {
      uint64_t read_in_flight_data = (uint64_t)read_in_flight;
      serializeInt(read_in_flight_data, length, data);
    }
    if (tx_type == 2) {
      uint64_t burst_in_flight_data = (uint64_t)(burst_in_flight);
      serializeInt(burst_in_flight_data, length, data);
    }
  }
  if (addr == (io_coh_helper_base + 0x580)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] read request  programmed read size: {:#x} read counter {:#x} \n", tx_size, read_counter);
    // serializeInt(backdoor_read_data, length, data);
    if (read_counter < tx_size) {
      data[0] = rdata_byte_vec[read_counter];
      read_counter++;
    } else {
      cvm::log(cvm::ERROR, "Error:[io_coh_helper] read request exceeded programmed read size: {:#x} \n", tx_size);
    }
  }
  if (addr == (io_coh_helper_base + 0x5b0)) {
    uint64_t backdoor_write_status = (uint64_t)write_in_flight;
    serializeInt(backdoor_write_status, length, data);
  }

  return;
}

io_coh_helper::~io_coh_helper() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"io_coh_helper_num_writes\": \"{}\"}}\n", num_writes);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"io_coh_helper_num_reads\": \"{}\"}}\n", num_reads);
  }
}

void io_coh_helper::checkUsage() {
}

void io_coh_helper::gen_data_strb(uint64_t addr, data_t& wdata, std::vector<bool>& strb) {
  uint8_t b_index = static_cast<uint8_t>(addr & 0x3F);

  for (uint8_t i = 0; i < 64; ++i) {
    wdata.push_back(0x0);
    strb.push_back(0x0);
  }

  for (uint8_t i = 0; i < tx_size; i++) {
    if ((i + b_index) < 63) {
      wdata[i + b_index] = wdata_vec[i];
      strb[i + b_index] = 0x1;
    }
  }
}

void io_coh_helper::overlay_write(uint64_t addr) {

  cvm::log(cvm::FULL, "[io_coh_helper] axi write addr= {:#X}   \n", addr);
  uint64_t waddr = addr;

  auto* l = +[](uint64_t waddr, io_coh_helper* dev) -> cvm::messenger::task<void> {
    co_await dev->blocking_write(waddr);
  };
  cvm::registry::messenger.fork(l, waddr, this);
}
cvm::messenger::task<void> io_coh_helper::blocking_write(uint64_t addr) {
  int hart = 0;
  bool valid;
  axi::a_t aw_txn;
  aw_txn.w = true;
  aw_txn.id = axi_id++;
  aw_txn.addr = addr;
  aw_txn.len = 0;
  aw_txn.size = log2(tx_size); //3;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock = 0;
  aw_txn.cache = axi::cache_mem_attr_t(0);
  //  Set PROT based on 55th bit
  aw_txn.prot = (addr & (1ULL << 55)) ? 0 : 2;
  aw_txn.qos = 0;
  aw_txn.region = 0;
  aw_txn.atop = 0;
  aw_txn.user = io_coh_user_bits | 8;
  aw_txn.allow_decerr_resp = FLAGS_io_coherency_disable || (aw_txn.addr & 0x3) || (aw_txn.size == 0 || aw_txn.size == 1) || ((aw_txn.addr & 0x7) == 4 && aw_txn.size >= 3);

  cvm::log(cvm::LOW, "[io_coh_helper] SP_XTOR AXI MMR WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  cvm::topology::loc_t axi_loc = mst_for_addr(addr);
  cvm::log(cvm::HIGH, "[io_coh_helper] overlay AW master={} mmr={} id={} addr={:#x} size={} burst={} len={}\n",
           cvm::topology::name(axi_loc), is_mmr_window(addr), aw_txn.id, aw_txn.addr,
           unsigned(aw_txn.size), unsigned(aw_txn.burst), aw_txn.len);
  cvm::registry::messenger.signal(axi_loc, aw_txn);
  axi::w_t w_txn;
  std::vector<uint8_t> data_vec;
  std::vector<bool> strb_vec;
  gen_data_strb(addr, data_vec, strb_vec);
  for (uint8_t i = 0; i < 64; ++i) {
    w_txn.data.push_back(data_vec[i]);
    w_txn.strb.push_back(strb_vec[i]);
  }

  w_txn.last = 1;
  uint32_t wresp_id = aw_txn.id;
  cvm::registry::messenger.signal(axi_loc, w_txn);
  //cvm::topology::loc_t axi_mst_loc_lambda = axi_mst_loc_l;

  write_in_flight = true;
  //auto t = std::make_tuple(axi_mst_loc_lambda, std::ref(write_in_flight));
  //auto t = std::make_tuple(wresp_channel, std::ref(write_in_flight),wresp_id);
  //auto* l = +[](decltype(t) t) -> cvm::messenger::task<void>{
  //co_await cvm::registry::messenger.wait<transactor::write_response_t>(std::get<0>(t));
  //auto response =
  //auto id = std::get<2>(t);
  //auto wresp_channel_l = std::get<0>(t);
  //co_await cvm::registry::messenger.wait<read_response_t>(resp_channel_, [&id] (const read_response_t& r) { return r.id == id; });
  //co_await cvm::registry::messenger.wait<axi::b_t>(wresp_channel, [&wresp_id] (const axi::b_t& wresp) { return wresp.id == wresp_id; });

  axi::b_t wresp = co_await cvm::registry::messenger.wait<axi::b_t>(
      b_channel_for(axi_loc),
      [&wresp_id](const axi::b_t& b) { return b.id == wresp_id; });
  cvm::log(cvm::HIGH, "[io_coh_helper] overlay B id={} resp={}\n", wresp.id, uint8_t(wresp.resp));

  if (!aw_txn.allow_decerr_resp && wresp.resp != axi::RESP_OKAY) {
    cvm::log(cvm::ERROR, "Error: Bad write completion response {} \n", +wresp.resp);
    co_return;
  }
  //std::get<1>(t) = false;
  write_in_flight = false;
  //}//;
  //cvm::registry::messenger.fork(l, t);
  if (wresp.resp == axi::RESP_OKAY) {
    //Poke same data to whisper memory
    cvm::log(cvm::MEDIUM, "[io_coh_helper] Backdoor whisper poke addr{:#x} poke_data {:#x} \n", addr, data_vec[0]);
    for (uint8_t i = 0; i < tx_size; ++i) {
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 0, 'm', addr + i, 1, data_vec[i], false, false, valid) || !valid) && FLAGS_whisper_client_check) {
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
        co_return;
      }
      {

        cvm::log(cvm::MEDIUM, "[io_coh_helper] backdoor whisper poke  Successful for addr{:#x} poke_data {:#x} \n", addr + i, data_vec[i]);
      }
    }
    num_writes++;
  } else {
    cvm::log(cvm::NONE, "[io_coh_helper] Backdoor whisper poke NOT DONE because of bad response addr{:#x} \n", addr);
  }
  co_return;
}

void io_coh_helper::drive_burst() {
  cvm::log(cvm::FULL, "[io_coh_helper] drive burst    \n");

  auto* l = +[](io_coh_helper* dev) -> cvm::messenger::task<void> {
    co_await dev->blocking_burst_thread();
  };
  cvm::registry::messenger.fork(l, this);
}

void io_coh_helper::overlay_read(uint64_t addr) {
  cvm::log(cvm::FULL, "[io_coh_helper] axi read addr= {:#X}   \n", addr);
  transactor::read_t r;
  r.addr = addr;
  r.length = 0x40;
  auto* l = +[](transactor::read_t r, io_coh_helper* dev) -> cvm::messenger::task<void> {
    data_t d;
    co_await dev->blocking_read(r, d);
  };
  cvm::registry::messenger.fork(l, r, this);
}

cvm::messenger::task<void> io_coh_helper::blocking_read(const transactor::read_t& r, data_t&) {

  axi::a_t ar_txn;
  ar_txn.w = false;
  ar_txn.id = axi_id++;
  //ar_txn.addr = 0x60000000;
  ar_txn.addr = r.addr;
  ar_txn.len = 0;
  //ar_txn.size = 6;
  ar_txn.size = log2(tx_size);
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock = 0;
  ar_txn.cache = axi::cache_mem_attr_t(0);
  //  Set PROT based on 55th bit
  ar_txn.prot = (r.addr & (1ULL << 55)) ? 0 : 2;
  ar_txn.qos = 0;
  ar_txn.region = 0;
  ar_txn.atop = 0;
  ar_txn.user = io_coh_user_bits;
  ar_txn.allow_decerr_resp = FLAGS_io_coherency_disable || (ar_txn.addr & 0x3) || (ar_txn.size == 0 || ar_txn.size == 1) || ((ar_txn.addr & 0x7) == 4 && ar_txn.size >= 3);

  cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data begin: \n");

  read_in_flight = true;
  rdata_byte_vec = {};

  cvm::topology::loc_t axi_loc = mst_for_addr(r.addr);
  cvm::log(cvm::HIGH, "[io_coh_helper] overlay AR master={} mmr={} id={} addr={:#x} size={} burst={} len={}\n",
           cvm::topology::name(axi_loc), is_mmr_window(r.addr), ar_txn.id, ar_txn.addr,
           unsigned(ar_txn.size), unsigned(ar_txn.burst), ar_txn.len);
  cvm::registry::messenger.signal(axi_loc, ar_txn);

  uint32_t rresp_id = ar_txn.id;
  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(
      r_channel_for(axi_loc),
      [&rresp_id](const axi::r_t& r) { return r.id == rresp_id; });
  cvm::log(cvm::HIGH, "[io_coh_helper] overlay R id={} resp={} last={}\n", resp.id, uint8_t(resp.resp), resp.last);

  cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data begin: \n");
  backdoor_read_data = 0;
  read_counter = 0;
  for (size_t i = 0; i < tx_size; ++i) {
    rdata_byte_vec.push_back(uint8_t(resp.data[i]));
    cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data[{}] = {}: \n", i, uint32_t(rdata_byte_vec[i]));
  }
  read_in_flight = false;
  num_reads++;
  co_return;
}

cvm::messenger::task<void> io_coh_helper::blocking_burst_thread() {
  bool valid;
  int hart = 0;

  burst_in_flight = true;
  for (int i = 0; i < int(txns_vec.size()); i++) {
    axi::a_t a_txn;
    a_txn.w = txns_vec[i].r0_w1;
    a_txn.id = axi_id++;
    a_txn.addr = txns_vec[i].addr;
    //ar_txn.addr = r.addr;
    a_txn.len = 0;
    //ar_txn.size = 6;
    a_txn.size = txns_vec[i].size;
    a_txn.burst = axi::burst_t(0);
    a_txn.lock = 0;
    a_txn.cache = axi::cache_mem_attr_t(0);
    a_txn.prot = 2;
    //  Set PROT based on 55th bit
    a_txn.prot = (txns_vec[i].addr & (1ULL << 55)) ? 0 : 2;
    a_txn.qos = 0;
    a_txn.region = 0;
    a_txn.atop = 0;
    a_txn.user = io_coh_user_bits;
    a_txn.allow_decerr_resp = FLAGS_io_coherency_disable || (a_txn.addr & 0x3) || (a_txn.size == 0 || a_txn.size == 1) || ((a_txn.addr & 0x7) == 4 && a_txn.size >= 3);

    cvm::log(cvm::HIGH, "[io_coh_helper] blocking burst data begin: \n");

    read_in_flight = true;
    rdata_byte_vec = {};

    cvm::topology::loc_t axi_loc = mst_for_addr(txns_vec[i].addr);
    cvm::log(cvm::HIGH, "[io_coh_helper] overlay {} master={} mmr={} id={} addr={:#x} size={} burst={} len={}\n",
             txns_vec[i].r0_w1 ? "AW" : "AR", cvm::topology::name(axi_loc), is_mmr_window(txns_vec[i].addr),
             a_txn.id, a_txn.addr, unsigned(a_txn.size), unsigned(a_txn.burst), a_txn.len);
    cvm::registry::messenger.signal(axi_loc, a_txn);
    if (txns_vec[i].r0_w1 == 0) {
      uint32_t rresp_id = a_txn.id;
      auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(
          r_channel_for(axi_loc),
          [&rresp_id](const axi::r_t& r) { return r.id == rresp_id; });
      cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data begin: \n");
      backdoor_read_data = 0;
      read_counter = 0;
      //for (size_t i = 0; i < 8; ++i) {
      for (size_t i = 0; i < tx_size; ++i) {
        //backdoor_read_data |= static_cast<uint64_t>(resp.data[i]) << (8 *  i);
        rdata_byte_vec.push_back(uint8_t(resp.data[i]));
        cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data[{}] = {}: \n", i, uint32_t(rdata_byte_vec[i]));
      }

      num_reads++;
      read_in_flight = false;

    } else {

      axi::w_t w_txn;
      std::vector<uint8_t> data_vec;
      std::vector<bool> strb_vec;

      uint8_t b_index = static_cast<uint8_t>(txns_vec[i].addr & 0x3F);

      for (uint8_t i = 0; i < 64; ++i) {
        data_vec.push_back(0x0);
        strb_vec.push_back(0x0);
      }

      for (uint8_t i = 0; i < txns_vec[i].size; i++) {
        if ((i + b_index) < 63) {
          data_vec[i + b_index] = txns_vec[i].wdata_byte_vec[i];
          strb_vec[i + b_index] = 0x1;
        }
      }
      for (uint8_t i = 0; i < 64; ++i) {
        w_txn.data.push_back(data_vec[i]);
        w_txn.strb.push_back(strb_vec[i]);
      }

      w_txn.last = 1;
      cvm::registry::messenger.signal(axi_loc, w_txn);
      //co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_l);

      /////-----------------------------
      uint32_t wresp_id = a_txn.id;
      //co_await cvm::registry::messenger.wait<read_response_t>(resp_channel_, [&id] (const read_response_t& r) { return r.id == id; });
      if (blocking_mode) {
        axi::b_t wresp = co_await cvm::registry::messenger.wait<axi::b_t>(
            b_channel_for(axi_loc),
            [&wresp_id](const axi::b_t& b) { return b.id == wresp_id; });

        if (!a_txn.allow_decerr_resp && wresp.resp != axi::RESP_OKAY) {
          cvm::log(cvm::ERROR, "Error: Bad write completion response {} \n", +wresp.resp);
          co_return;
        }
        ////------------------------------
        if (wresp.resp == axi::RESP_OKAY) {
          //Poke same data to whisper memory
          cvm::log(cvm::MEDIUM, "[io_coh_helper] Backdoor whisper poke burst mode addr{:#x} poke_data {:#x} \n", txns_vec[i].addr, data_vec[0]);
          for (uint8_t i = 0; i < tx_size; ++i) {
            if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 0, 'm', txns_vec[i].addr + i, 1, data_vec[i], false, false, valid) && FLAGS_whisper_client_check)) {
              cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
              co_return;
            }
            {

              cvm::log(cvm::HIGH, "[io_coh_helper] backdoor whisper poke  Successful for addr{:#x} poke_data {:#x} \n", txns_vec[i].addr + i, data_vec[i]);
            }
          }
          num_writes++;
        } else {
          cvm::log(cvm::NONE, "[io_coh_helper] Backdoor whisper poke NOT DONE  because of bad response burst mode addr{:#x} poke_data {:#x} \n", txns_vec[i].addr, data_vec[0]);
        }
      }
    }
  }
  burst_in_flight = false;
  co_return;
}

void io_coh_helper::write(uint64_t addr, size_t, const data_t& data,
                          const strb_t&) {
  if (not has_addr(addr)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Descarding write request at io_coh_helper since tag {} is not matching \n", tag());
    return;
  }
  cvm::log(cvm::HIGH, "[io_coh_helper] write addr {:#x}  \n", addr);
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[io_coh_helper] write data {:#x} \n", t_data);

  if (addr == io_coh_helper_base) {
    if (t_data > 0)
      cvm::log(cvm::ERROR, "Error:[io_coh_helper] Only Clearing of io_coh_helper Status allowed, Illegal to set status bit manually \n");
    tx_status = t_data & 0x1;
    wdata_vec = {};

  } else if (addr == (io_coh_helper_base + 0x080)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Burst Transfer latch Addr {:#x} type_size {:#x}\n", tx_addr_burst, tx_type_size);
    //tx_type_size = (uint8_t)t_data;
    axi_txns tx;
    tx.addr = tx_addr_burst;
    if ((tx_type_size >> 8) > 0) {
      tx.r0_w1 = true;
    } else {
      tx.r0_w1 = false;
    }
    tx.size = tx_type_size & 0x0f;
    tx.wdata_byte_vec = wdata_vec;
    txns_vec.push_back(tx);
  } else if (addr == (io_coh_helper_base + 0x100)) {
    tx_addr = t_data;
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer Start Addr {:#x} \n", t_data);
    tx_addr_burst = t_data;

  } else if (addr == (io_coh_helper_base + 0x180)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Burst Transfer Size {:#x} \n", t_data);
    tx_type_size = (uint8_t)t_data;
  } else if (addr == (io_coh_helper_base + 0x200)) {
    tx_data0 = t_data;
    wdata_vec.push_back(uint8_t(t_data));
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer wdata {:#x}  \n", t_data);
  } else if (addr == (io_coh_helper_base + 0x300)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer type {:#x}  \n", t_data);
    tx_type = t_data;

    if (tx_type == 2) {
      //create array of structs
      txns_vec = {};
    }

  } else if (addr == (io_coh_helper_base + 0x340)) {
    io_coh_user_bits = t_data & 0xFF;
    cvm::log(cvm::HIGH, "[io_coh_helper] Setting User bits to  {:#x}  \n", io_coh_user_bits);

  } else if (addr == (io_coh_helper_base + 0x380)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Blocking/Non Blocking type {:#x}  \n", t_data);
    if (t_data == 1) {
      cvm::log(cvm::HIGH, "[io_coh_helper]  Set to Non Blocking type  \n");
      blocking_mode = false;
    } else {
      cvm::log(cvm::HIGH, "[io_coh_helper]  Set to Blocking type  \n");
      blocking_mode = true;
    }

  } else if (addr == (io_coh_helper_base + 0x400)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer trigger {:#x}  \n", t_data);

    if (tx_type == 0) {
      if (wdata_vec.size() != tx_size) {
        cvm::log(cvm::ERROR, "Error: [io_coh_helper] wdata vector size doesnt match programmed size \n");

      } else {
        overlay_write(tx_addr);
      }
    } else if (tx_type == 1) {
      backdoor_read_data = 0;
      overlay_read(tx_addr);
    } else if (tx_type == 2) {
      drive_burst();
    }

  } else if (addr == (io_coh_helper_base + 0x600)) {
    tx_size = t_data;
  }
}
