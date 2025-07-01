#pragma once

#include "sysmod/device.h"
#include <string>
#include <mem_manager.h>
#include <memory>

class sysmod_mem : public device {

    private:

        std::shared_ptr<mem_manager> m_;

    public:
        void write(const transactor::write_t& w);

        void read(const transactor::read_t& r, data_t& data);

        virtual void backdoor_write(uint64_t addr, size_t length, data_t& data, strb_t& strb) override;

        virtual void backdoor_read(uint64_t addr, size_t length, data_t& data) override;

        // add max mem size
        sysmod_mem(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, std::shared_ptr<mem_manager> m)
          : device(tag, addr, size, loc, &sysmod_mem::write, &sysmod_mem::read, this), m_(m) {}

        /// Initialize memory with hex file.
        bool init_hex(const std::string& path);

        /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

        /// Initialize memory with LZ4 compressed file.
        bool init_lz4(const std::string& path, uint64_t offset = 0);

        /// Initialize memory with binary file.
        bool init_bin(const std::string& path, uint64_t offset = 0);

        /// Set callback for uninitialized memory reads.
        void uninitialized_read_data_cb(std::function<std::vector<std::uint8_t>(std::uint64_t, std::uint64_t)> cb);
};
