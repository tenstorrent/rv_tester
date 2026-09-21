// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <cstdint>
#include <elf.h>
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
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return false;
  }
  return true;
}

namespace {

// Load every PT_LOAD segment from an ELF image (already read into `buf`) into
// `m` at its full physical address. Returns false if the headers don't parse,
// so the caller can fall back to the legacy loader.
template <typename Ehdr, typename Phdr>
bool load_elf_pt_load(const std::vector<std::uint8_t>& buf, mem_manager* m) {
  if (buf.size() < sizeof(Ehdr))
    return false;
  Ehdr eh;
  std::memcpy(&eh, buf.data(), sizeof(eh));
  if (eh.e_phoff == 0 || eh.e_phnum == 0 || eh.e_phentsize < sizeof(Phdr))
    return false;
  for (unsigned i = 0; i < eh.e_phnum; ++i) {
    const std::size_t off = static_cast<std::size_t>(eh.e_phoff) + static_cast<std::size_t>(i) * eh.e_phentsize;
    if (off + sizeof(Phdr) > buf.size())
      return false;
    Phdr ph;
    std::memcpy(&ph, buf.data() + off, sizeof(ph));
    if (ph.p_type != PT_LOAD || ph.p_filesz == 0)
      continue;
    if (static_cast<std::size_t>(ph.p_offset) + ph.p_filesz > buf.size())
      return false;
    // Write at the full 64-bit physical address. (p_memsz > p_filesz bytes are
    // .bss and read back as zero via the uninitialized-read callback.)
    m->write(static_cast<std::uint64_t>(ph.p_paddr), ph.p_filesz, buf.data() + ph.p_offset);
  }
  return true;
}

} // namespace

bool sysmod_mem::init_elf(const std::string& path) {
  try {
    // Load PT_LOAD segments at their full 64-bit PA. mem::load_ELF() shells out
    // to `objcopy -O verilog`, whose @address records are truncated to 32 bits,
    // silently dropping sections above 4GB (e.g. .os_data) for the DUT while the
    // ISS loads the ELF directly and sees the real data.
    std::ifstream file(path, std::ios::binary);
    if (file) {
      std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      if (buf.size() >= EI_NIDENT && std::memcmp(buf.data(), ELFMAG, SELFMAG) == 0 && buf[EI_DATA] == ELFDATA2LSB) {
        bool ok = false;
        if (buf[EI_CLASS] == ELFCLASS64)
          ok = load_elf_pt_load<Elf64_Ehdr, Elf64_Phdr>(buf, m_.get());
        else if (buf[EI_CLASS] == ELFCLASS32)
          ok = load_elf_pt_load<Elf32_Ehdr, Elf32_Phdr>(buf, m_.get());
        if (ok)
          return true;
      }
    }
    // Fallback for anything the direct parser can't handle (e.g. big-endian).
    m_->load_ELF(path);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return false;
  }
  return true;
}

bool sysmod_mem::init_lz4(const std::string& path, uint64_t offset) {
  try {
    m_->load_lz4(path, offset);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return false;
  }
  return true;
}

bool sysmod_mem::init_bin(const std::string& path, uint64_t offset) {
  try {
    m_->load_bin(path, offset);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return false;
  }
  return true;
}

void sysmod_mem::uninitialized_read_data_cb(std::function<std::vector<std::uint8_t>(std::uint64_t, std::uint64_t)> cb) {
  m_->uninitialized_read_data_cb(cb);
}
