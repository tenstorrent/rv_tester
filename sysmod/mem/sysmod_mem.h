#pragma once

#include "sysmod/device.h"
#include <string>
#include <mem_manager.h>

class sysmod_mem : public device {

    private:

        mem_manager m_;

    public:
        void write(const transactor::write_t& w);

        void read(const transactor::read_t& r, data_t& data);

        virtual void backdoor_write(uint64_t addr, size_t length, data_t& data, strb_t& strb) override;

        virtual void backdoor_read(uint64_t addr, size_t length, data_t& data) override;

        // add max mem size
        sysmod_mem(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc)
          : device(tag, addr, size, loc, &sysmod_mem::write, &sysmod_mem::read, this) { }

        /// Initialize memory with hex file.
        bool init_hex(const std::string& path);

        /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
