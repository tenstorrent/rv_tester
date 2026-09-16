#include "src/sysmod/trickbox/debug_req_vld.h"

debug_req_vld::debug_req_vld(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
    : subdevice(tag, addr, 0x1000 /* size */, loc),
      assert_addr_(addr), deassert_addr_(addr + 0x8),
      hart_count_(hartCount), loc_(loc) {
  cvm::log(cvm::HIGH, "[debug_req_vld] Constructor: hart_count = {} assert_addr = {:#x} deassert_addr = {:#x}\n",
           hart_count_, assert_addr_, deassert_addr_);
  reset();
}

debug_req_vld::~debug_req_vld() {}

void debug_req_vld::write(uint64_t addr, size_t, const data_t& data,
                          const strb_t&) {
  if (not has_addr(addr))
    return;

  uint64_t t_data = 0;
  deserializeInt(data, t_data);

  if (addr != assert_addr_ && addr != deassert_addr_) {
    cvm::log(cvm::ERROR, "Error:[debug_req_vld] Unknown write to addr {:#x} data={:#x}\n", addr, t_data);
    return;
  }

  const bool set = (addr == assert_addr_);
  const uint64_t hart = t_data & 0xff;
  if (hart >= hart_count_) {
    cvm::log(cvm::ERROR, "Error:[debug_req_vld] hart {} out of range, hart_count={}\n", hart, hart_count_);
    return;
  }

  if (set)
    req_mask_ |= (1ull << hart);
  else
    req_mask_ &= ~(1ull << hart);

  cvm::log(cvm::MEDIUM, "[debug_req_vld] debug request {} for hart {}, mask={:#x}\n", set, hart, req_mask_);
  cvm::registry::messenger.signal(loc_, request_t{hart, set});
}

void debug_req_vld::read_dev(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr))
    return;
  if (addr == assert_addr_)
    serializeInt(req_mask_, length, data);
}
