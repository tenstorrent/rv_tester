#pragma once

#include <mutex>
#include "device.h"
#include "svdpi.h"

class sysmod {

  public:

    sysmod(svScope& scope, const std::string& spec);

    ~sysmod();

    device& dev(uint64_t addr);
    device& dev(const std::string& tag);

    void compose(const std::string& spec);

    void load_prog(const std::string& prog);
    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb);
    void read(uint64_t addr, size_t length, device::data_t& data);
    void tick();
    void flush_cbs();

  private:

    void handle_callbacks(const device::cbs_t& cbs);

    svScope scope_;

    mutable std::mutex sys_m;
    std::vector<device*> devices_;

    // queue up callbacks for emu to flush later (need main thread to call DPI)
    std::vector<device::cb_t> callbacks_;
};
