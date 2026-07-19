// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include "src/sysmod/device.h"
#include "IoDevice.hpp"

// io_device wraps a Whisper IoDevice and allows us to use it with the DUT in
// rv_tester

// This base class allows us to initialize the member io_device before calling
// the superclass device's constructor in io_device. We want to do it in this
// order so we can pass the address and size from the IoDevice to the device
// constructor
template <std::derived_from<WdRiscv::IoDevice> T>
class io_device_base {
protected:
  T whisper_device_;

  template <typename... Us>
  io_device_base(Us&&... args) : whisper_device_(std::forward<Us>(args)...) {}
};

template <std::derived_from<WdRiscv::IoDevice> T>
class io_device : private io_device_base<T>, public device {
public:
  template <typename... Us>
  io_device(std::string tag, cvm::topology::loc_t loc, Us&&... args) : io_device_base<T>(std::forward<Us>(args)...), device(tag, this->whisper_device_.address(),
                                                                                                                            this->whisper_device_.size(), loc, &io_device<T>::write, &io_device<T>::read, this) {}

  void read(const transactor::read_t& r, data_t& data) {
    uint64_t addr = r.addr;
    size_t size = r.length;
    uint32_t value = this->whisper_device_.read(addr);
    serializeInt(value, size, data);
  }

  void write(const transactor::write_t& w) {
    uint64_t addr = w.addr;
    size_t size = w.length;
    uint32_t value;
    deserializeInt(w.data, value);
    value &= (uint64_t(1) << (size * 8)) - 1;
    this->whisper_device_.write(addr, value);
  }

private:
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
};
