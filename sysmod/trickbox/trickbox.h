// -*- c++ -*-

#include "device.h"
#pragma once
#include "Interruptor.h"
/// Model an trickbox (host target interface) device
class trickbox : public device
{
public:

  trickbox(const std::string& tag, uint64_t addr);

  virtual ~trickbox();
// Used to assert/deassert a software interrupt (PIPI) for given hart.
  virtual void trickboxInterrupt(unsigned hart, bool flag,unsigned event, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TRICKBOX_EVT, hart, flag,event,0});
  }

  // Used to assert/deassert a timer interrupt for given hart.
  virtual void trickboxDelayedInterrupt(unsigned hart, bool flag,unsigned event, unsigned delay, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TRICKBOX_EVT, hart, flag,event,delay});
  }

  // Reads outside of device range are ignored. Reads with length
  // different than 8 are ignored.
  virtual void read(uint64_t addr, size_t length, data_t& data,
                    cbs_t& cbs) override;

  // Writes outside of device range are ignored. Writes with length
  // different than 8 are ignored.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
		     const strb_t& strb, cbs_t& cbs) override;
  //extern virtual void handle_itp(uint64_t offset,uint64_t dword,cbs_t& cbs);
  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
  }

  Interruptor* ic;
private:

  uint64_t to_ = 0;
  uint64_t from_ = 0;
};
