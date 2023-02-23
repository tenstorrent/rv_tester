#pragma once

#include "device.h"
#include <string>

class null_dev : public device {

    private:


    public:
        virtual void write(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb,
                            cbs_t& cbs) override;

        virtual void read(uint64_t addr, size_t length,
                          data_t& data, cbs_t& cbs) override;

        // add max mem size
        null_dev(const std::string& tag, const std::string& type, uint64_t addr, size_t size) : device(tag, type, addr, size) {}


};
