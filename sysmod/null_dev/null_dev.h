#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>

class null_dev : public device {

    private:


    public:
        virtual void write(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb) override;

        virtual cvm::messenger::task<void> read(uint64_t addr, size_t length,
                                                data_t& data) override;

        // add max mem size
        null_dev(const std::string& tag, uint64_t addr, size_t size) : device(tag, addr, size, cvm::topology::null) {}


};
