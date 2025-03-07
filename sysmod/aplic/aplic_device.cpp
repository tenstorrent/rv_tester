#include "aplic_device.h"

void
aplic_device::read(const transactor::read_t& r, data_t& data)
{
  uint64_t addr = r.addr;
  size_t size = r.length;
  uint32_t value;

  aplic_->read(addr, size, value);
  serializeInt(value, size, data);
}


void
aplic_device::write(const transactor::write_t& w)
{
  uint64_t addr = w.addr;
  size_t size = w.length;
  uint32_t value;
  deserializeInt(w.data, value);

  aplic_->write(addr, size, value);
}
