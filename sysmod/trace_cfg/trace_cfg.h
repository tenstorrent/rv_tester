#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <mem_manager.h>
#include "cvm/registry.hpp"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"

class trace_cfg : public device {

    private:

        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;

    public:
        virtual void axi_write();
        cvm::messenger::task<void> axi_read();
        void write(const transactor::write_t& );

        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );

        virtual void write_axi_mst(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb);

        virtual void read_axi_mst(uint64_t addr, size_t length,
                          data_t& data);
        virtual void tick(uint64_t) override
        {
            cvm::log(cvm::HIGH, "[Trickbox] trace_cfg timer tick advance interval \n");
            //processDelayedRandomInterrupts();
        	axi_write();
        	axi_read();
        
        }
        // add max mem size
        trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
