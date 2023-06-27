#pragma once

#include "sysmod/device.h"
#include <string>
#include <mem_manager.h>

class sysmod_mem : public device {

    private:

        mem_manager m_;

    public:
        virtual void write(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb) override;

        virtual cvm::messenger::task<void> read(uint64_t addr, size_t length,
                                                data_t& data) override;

        virtual void backdoor_read(uint64_t addr, size_t length, data_t& data) override;

        // add max mem size
        sysmod_mem(const std::string& tag, uint64_t addr, size_t size)
                    : device(tag, addr, size, cvm::topology::null) {}

        /// Initialize memory with hex file.
        bool init_hex(const std::string& path);

        /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
