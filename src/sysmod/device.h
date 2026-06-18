#pragma once

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <sstream>
#include "transactor.h"
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"

template <typename T, typename U>
concept not_same_as = !std::same_as<T, U>;

class device {
public:
  typedef std::vector<uint8_t> data_t;
  typedef std::vector<bool> strb_t;

  // need to create a new type, no way to define new type and we dynamically elaborate devices
  struct write_t {
    transactor::write_t w;
  };
  struct read_t {
    transactor::read_t r;
    cvm::topology::loc_t source;
  };

private:
  const std::string tag_;
  const uint64_t addr_;
  const size_t size_;
  const cvm::topology::loc_t loc_;
  std::function<void()> configure_ = nullptr;

  template <typename U, typename V>
    requires std::invocable<U, V, transactor::write_t>
  inline void spawn_write_thread(U write, V* dev) {
    cvm::registry::messenger.connect<write_t>(
        loc_,
        [write, dev](const auto& w) {
          std::string d, s;
          if (cvm::logger::check_verbosity(cvm::FULL))
            for (int i = 63; i >= 0; i--) {
              d += fmt::format("{:02x}", w.w.data[i]);
              s += fmt::format("{:01x}", w.w.strb[i] ? 1 : 0);
            }
          cvm::log(cvm::FULL, "[device] tag={}: aw/w: addr={:#x}, len={}, strb={}, data={}\n", dev->tag(), w.w.addr, w.w.length, s, d);
          return std::invoke(write, dev, w.w);
        },
        [dev](const auto& w) { return dev->has_addr(w.w.addr); });
  }

  template <typename U, typename V>
    requires requires(U read, V* dev, transactor::read_t r, device::data_t d) {{ std::invoke(read, dev, r, d) } -> not_same_as<cvm::messenger::task<>>; }
  inline void spawn_read_thread(U read, V* dev) {
    cvm::registry::messenger.connect<read_t>(
        loc_,
        [read, dev](const auto& r) {
          data_t data(r.r.length, 0);
          std::invoke(read, dev, r.r, data);
          std::string d;
          if (cvm::logger::check_verbosity(cvm::FULL))
            for (int i = 63; i >= 0; i--)
              d += fmt::format("{:02x}", data[i]);
          cvm::log(cvm::FULL, "[device] tag={}: src={}: r: id={}, addr={:#x}, len={}, size={}, data={}\n", dev->tag(), r.source, r.r.id, r.r.addr, r.r.length, data.size(), d);
          cvm::registry::messenger.signal(r.source, transactor::read_response_t{r.r.id, std::move(data)});
        },
        [dev](const auto& r) { return dev->has_addr(r.r.addr); });
  }

  template <typename U, typename V>
    requires requires(U read, V* dev, transactor::read_t r, device::data_t d) {{ std::invoke(read, dev, r, d) } -> std::same_as<cvm::messenger::task<>>; }
  inline void spawn_read_thread(U read, V* dev) {
    auto* l = +[](cvm::topology::loc_t loc_, U read, V* dev) -> cvm::messenger::task<void> {
      auto channel = cvm::registry::messenger.channel<read_t>(loc_);

      while (1) {
        auto r = co_await cvm::registry::messenger.wait<read_t>(channel, [dev](const auto& r) { return dev->has_addr(r.r.addr); });
        data_t data(r.r.length, 0);
        co_await std::invoke(read, dev, r.r, data);
        cvm::registry::messenger.signal(r.source, transactor::read_response_t{r.r.id, std::move(data)});
      }
      co_return;
    };
    cvm::registry::messenger.fork(l, loc_, std::forward<U>(read), std::forward<V*>(dev));
  }

public:
  bool load_snapshot(std::ifstream& ifs);

  bool save_snapshot(const std::stringstream& ss);

  virtual void backdoor_write(uint64_t, size_t, data_t&, strb_t&) {};

  virtual void backdoor_read(uint64_t, size_t, data_t&) {};

  virtual void tick(uint64_t) {};
  virtual void is_dut_reset_req(bool, uint64_t, uint64_t) {};
  virtual void jtag_tick(uint64_t) {};
  virtual void overlay_tick(uint64_t) {};

  virtual void reset() {};

  device(std::string tag, uint64_t addr, size_t size, cvm::topology::loc_t loc)
      : tag_(tag), addr_(addr), size_(size), loc_(loc) {};

  template <typename W, typename R, typename V>
  device(std::string tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, W write, R read, V* dev)
      : tag_(tag), addr_(addr), size_(size), loc_(loc) {
    configure_ = [this, write, read, dev]() {
      spawn_write_thread(write, dev);
      spawn_read_thread(read, dev);
    };
  };

  virtual ~device() {};

  void configure() {
    if (configure_)
      configure_();
  }

  std::string tag() const { return tag_; }
  uint64_t addr() const { return addr_; }
  size_t size() const { return size_; }
  cvm::topology::loc_t loc() const { return loc_; }

  bool has_addr(uint64_t val) const { return val >= addr_ && (val < (addr_ + size_)); }
};
