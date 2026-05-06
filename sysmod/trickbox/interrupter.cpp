#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "interrupter.h"
#include "sysmod/sysmod_plusargs.h"
#include "common/device_address_map/device_address_map.h"

interrupter::interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x50000 /* size */, loc),
    interrupter_base(addr), hart_count_(hartCount), loc_(loc)
{
  cvm::log(cvm::HIGH, "[Trickbox] Constructor: hart_count = {} interrupter_base = {:#x}\n", hart_count_, interrupter_base);

  checkUsage();

  cvm::registry::messenger.procedure<intr_enable_read_RPC>(loc, [this] (bool& enabled) {
    enabled = intr_enable_flag_;
    return true;
  });
}

interrupter::~interrupter() {}

uint64_t mnscratch_array[256];

void interrupter::read_dev(uint64_t addr, size_t length, data_t& data) {
  uint64_t mnscratch_base = interrupter_base + 0x2000;
  if ((addr >= mnscratch_base) && (addr < (mnscratch_base + 0x1000))) {
    int index = (addr - mnscratch_base) / 8;
    if (index < 256)
      serializeInt(mnscratch_array[index], length, data);
    else
      cvm::log(cvm::HIGH, "[Trickbox] IMSIC read - mnscratch index out of bounds: index={}\n", index);
  }
  return;
}

void interrupter::checkUsage() {}

cvm::messenger::task<void>
interrupter::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}

void
interrupter::write(uint64_t addr, size_t, const data_t& data,
                   const strb_t&)
{
  if (not has_addr(addr))
    return;
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  const uint64_t nmi_deassert_base = interrupter_base + 0x5000;
  const uint64_t nmi_stride = 0x100;

  if (addr == interrupter_base) {
    cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - addr={:#x} data={:#x}\n", addr, t_data);
    uint64_t intr_num = t_data & 0xfff;
    cvm::registry::messenger.signal(loc_, directed_msi_request_t{intr_num, t_data});
  }
  else if ((addr > interrupter_base) && (addr < (interrupter_base + 0x1000))) {
    cvm::log(cvm::HIGH, "[Trickbox] IMSIC write delayed - addr={:#x} data={:#x}\n", addr, t_data);
  }
  else if ((addr >= (interrupter_base + 0x2000)) && (addr < (interrupter_base + 0x3000))) {
    int index = (addr - (interrupter_base + 0x2000)) / 8;
    if (index >= 0 && index < 256)
      mnscratch_array[index] = t_data;
  }
  else if (addr == (interrupter_base + 0x4000)) {
    cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - no match - addr={:#x} data={:#x}\n", addr, t_data);
  }
  else if (addr == (interrupter_base + 0x4040)) {
    intr_enable_flag_ = (t_data != 0);
    cvm::log(cvm::MEDIUM, "[Trickbox] Interrupt enable flag set to {} at addr {:#x}\n", intr_enable_flag_, addr);
  }
  else if ((addr >= nmi_deassert_base) && (addr <= nmi_deassert_base + (hart_count_ * nmi_stride))) {
    uint64_t offset = addr - nmi_deassert_base;
    unsigned hart_id = offset / nmi_stride;
    auto target_loc = cvm::topology::get_from_type("INTERRUPTS", hart_id);
    cvm::registry::messenger.signal<uint8_t>(target_loc, 0);
    cvm::log(cvm::HIGH, "[Trickbox] NMI deassert for hart {} at addr {:#x}\n", hart_id, addr);
  }
  else {
    cvm::log(cvm::ERROR, "Error:[Trickbox] Unknown write to addr {:#x} data={:#x}\n", addr, t_data);
  }
}
