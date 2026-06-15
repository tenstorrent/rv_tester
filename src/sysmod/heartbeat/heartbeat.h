#pragma once

#include "src/sysmod/device.h"
#include <cstdint>

class heartbeat : public device {

    private:
        std::uint64_t count_ = 0;
        cvm::file_logger log_;

    public:

        heartbeat(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc) : device(tag, addr, size, loc), log_("heartbeat.log") {}

        virtual void tick(std::uint64_t advance) override;


};
