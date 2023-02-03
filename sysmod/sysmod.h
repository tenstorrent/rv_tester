#pragma once

#include <mutex>
#include <memory>
#include "device.h"
#include "endpoint.h"
#include "svdpi.h"
#include "memmap.h"

class sysmod : public endpoint {

  public:

    sysmod(int num);

    ~sysmod();

    device& dev(uint64_t addr);
    device& dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb) override;
    void read(uint64_t addr, size_t length, device::data_t& data) override;

    void compose();
    void load_prog();
    void tick(uint64_t advance);
    void reset();
    void set_scope(svScope s) { scope_ = s; }

    int num() { return num_; }
    svScope scope() { return scope_; }

  protected:

    void timer_interrupt(unsigned hart, bool flag);
    void sw_interrupt(unsigned hart, bool flag);
    void terminate();

  private:

    svScope scope_;
    int num_;

    mutable std::mutex sys_m;
    std::vector<std::unique_ptr<device> > devices_;

    // Memmap
    memmap::memmap_t memmap_;
};
