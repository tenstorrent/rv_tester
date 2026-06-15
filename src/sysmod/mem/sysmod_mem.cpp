#include <iostream>
#include "src/sysmod/mem/sysmod_mem.h"


void sysmod_mem::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;

  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_->write(addr + i, 1, &data[i]);
    }
  }
  return;
}

void sysmod_mem::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

  m_->read(addr, length, data.data());
  return;
}

void sysmod_mem::backdoor_write(uint64_t addr, size_t length, data_t& data, strb_t& strb) {
  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_->write(addr + i, 1, &data[i]);
    }
  }
  return;
}

void sysmod_mem::backdoor_read(uint64_t addr, size_t length, data_t& data) {
  m_->read(addr, length, data.data());
  return;
}

bool sysmod_mem::init_hex(const std::string& path) {
    try {
        m_->load_verilog_hex(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

bool sysmod_mem::init_elf(const std::string& path) {
    try {
        m_->load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

bool sysmod_mem::init_lz4(const std::string& path, uint64_t offset) {
    try {
        m_->load_lz4(path, offset);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

bool sysmod_mem::init_bin(const std::string& path, uint64_t offset) {
    try {
        m_->load_bin(path, offset);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

void sysmod_mem::uninitialized_read_data_cb(std::function<std::vector<std::uint8_t>(std::uint64_t, std::uint64_t)> cb)
{
  m_->uninitialized_read_data_cb(cb);
}
