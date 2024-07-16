#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"

class uart8250 : public device {

    public:

        uart8250(const std::string& tag, std::uint64_t address, cvm::topology::loc_t loc);
        void read(const transactor::read_t& r, data_t& data);
        void write(const transactor::write_t& w);

        void set_interrupt_pending(bool /* unused */) {}

    private:

        std::uint32_t byte_ = 0;  // Pending input byte.

        std::uint8_t ier_ = 0;     // Interrupt enable
        std::uint8_t iir_ = 1;     // Interrupt id
        std::uint8_t lcr_ = 0;     // Line control
        std::uint8_t mcr_ = 0;     // Modem control
        std::uint8_t lsr_ = 0x60;  // Line satus
        std::uint8_t msr_ = 0;     // Modem status
        std::uint8_t scr_ = 0;     // Scratch
        std::uint8_t fcr_ = 0;     // Fifo control
        std::uint8_t dll_ = 0x1;   // Divisor latch lsb
        std::uint8_t dlm_ = 0x1;   // Divisor latch msb
        std::uint8_t psd_ = 0;     // Pre-scaler division


};
