#pragma once

#include <mutex>
#include "device.h"
#include "endpoint.h"
#include "svdpi.h"

class sysmod : public endpoint {

  public:

    sysmod();

    ~sysmod();

    device& dev(uint64_t addr);
    device& dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb) override;
    void read(uint64_t addr, size_t length, device::data_t& data) override;

    void compose(const std::string& spec);
    void load_prog(const std::string& prog);
    void tick(uint64_t advance);
    void flush_cbs();
    void reset();
    void set_scope(svScope s) { scope_ = s; }

  private:

    void handle_callbacks(const device::cbs_t& cbs);

    svScope scope_;

    mutable std::mutex sys_m;
    std::vector<device*> devices_;

    // queue up callbacks for emu to flush later (need main thread to call DPI)
    std::vector<device::cb_t> callbacks_;
};
