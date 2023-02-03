// -*- c++ -*-

#include <boost/signals2.hpp>
#include <functional>
#include "device.h"

/// Model an htif (host target interface) device
class htif : public device
{
public:

  htif(const std::string& tag, const std::string& type, uint64_t addr);

  virtual ~htif();

  // Reads outside of device range are ignored. Reads with length
  // different than 8 are ignored.
  virtual void read(uint64_t addr, size_t length, data_t& data) override;

  // Writes outside of device range are ignored. Writes with length
  // different than 8 are ignored.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
		     const strb_t& strb) override;

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

  typedef std::function<void()> listener;
  void registerTerminate(const listener& l)
  {
    terminateSignal_.connect(l);
  }

private:

  uint64_t to_ = 0;
  uint64_t from_ = 0;

  boost::signals2::signal<void()> terminateSignal_;
};
