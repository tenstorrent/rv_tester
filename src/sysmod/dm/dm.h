#pragma once

#include "src/sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <mem_manager.h>
#include "cvm/registry.hpp"
#include "transactor.h"
#include "src/transactors/axi_sw/axi.h"

class dm : public device {

    private:

        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;

    public:
        void write(const transactor::write_t& w);

        cvm::messenger::task<void> read(const transactor::read_t& r, data_t& data);

        virtual void write_axi_mst(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb);

        virtual void read_axi_mst(uint64_t addr, size_t length,
                          data_t& data);
        // add max mem size
        dm(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);

        void configure();


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
