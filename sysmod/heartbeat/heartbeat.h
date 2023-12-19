#pragma once

#include "sysmod/device.h"
#include <cstdint>

class heartbeat : public device {

    private:
        std::uint64_t count_ = 0;

    public:

        heartbeat(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc) : device(tag, addr, size, loc) { }

        virtual void tick(std::uint64_t advance) override;


};
