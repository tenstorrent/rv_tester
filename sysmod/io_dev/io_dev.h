#pragma once

#include "device.h"
#include "cvm/topology.hpp"
#include <string>
#include <mem_manager.h>

class io_dev : public device {

    private:

        mem_manager m_;

    public:
        virtual void write(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb) override;

        virtual void read(uint64_t addr, size_t length,
                          data_t& data) override;

        // add max mem size
        io_dev(const std::string& tag, uint64_t addr, size_t size);

           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
