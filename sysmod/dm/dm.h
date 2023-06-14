#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <mem_manager.h>
#include "cvm/registry.hpp"
#include "transactor.h"

class dm : public device {

    private:

        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;

    public:
        virtual void write(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb) override;

        virtual void read(uint64_t addr, size_t length,
                          data_t& data) override;
        
        virtual void write_axi_mst(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb);

        virtual void read_axi_mst(uint64_t addr, size_t length,
                          data_t& data);
        // add max mem size
        dm(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);
        

           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
