#include "uart8250.h"

uart8250::uart8250(const std::string& tag, uint64_t addr, cvm::topology::loc_t loc)
  : device(tag, addr, 32 /* size */, loc, &uart8250::write, &uart8250::read, this) {
}

void uart8250::read(const transactor::read_t& r, data_t& data) {
    auto& addr = r.addr;
    auto& length = r.length;

    std::fill(data.begin(), data.end(), 0);

    std::uint64_t offset = (addr - this->addr()) / 4;
    bool dlab = lcr_ & 0x80;

    if (dlab == 0)
    {
        switch (offset)
        {
            case 0:
                {
                    uint32_t res = byte_;
                    byte_ = 0;
                    lsr_ &= ~1;  // Clear least sig bit
                    iir_ |= 1;   // Set least sig bit indicating no interrupt.
                    set_interrupt_pending(false);
                    data[0] = res;
                    break;
                }

            case 1: data[0] = ier_; break;
            case 2: data[0] = iir_; break;
            case 3: data[0] = lcr_; break;
            case 4: data[0] = mcr_; break;
            case 5: data[0] = lsr_; break;
            case 6: data[0] = msr_; break;
            case 7: data[0] = scr_; break;
            default:
                cvm::log(cvm::ERROR, "Error: could not process uart read to address 0x{:x} of size {}\n", addr, length);
        }
    }
    else
    {
        switch (offset)
        {
            case 0: data[0] = dll_; break;
            case 1: data[0] = dlm_; break;
            default:
                cvm::log(cvm::ERROR, "Error: could not process uart read to address 0x{:x} of size {}\n", addr, length);
        }
    }
}

void uart8250::write(const transactor::write_t& w)
{
    auto& addr  = w.addr;
    auto& value = w.data[0];

    std::uint64_t offset = (addr - this->addr()) / 4;
    bool dlab = lcr_ & 0x80;

    if (dlab == 0)
    {
        switch (offset)
        {
            case 0:
                {
                    int c = static_cast<int>(value & 0xff);
                    if (c)
                    {
                        putchar(c);
                        fflush(stdout);
                    }
                }
                break;

            case 1: ier_ = value; break;
            case 2: fcr_ = value; break;
            case 3: lcr_ = value; break;
            case 4: mcr_ = value; break;
            case 5:
            case 6: break;
            case 7: scr_ = value; break;
            default:
                    cvm::log(cvm::ERROR, "uart writing addr 0x{:x}\n", addr);
        }
    }
    else
    {
        switch (offset)
        {
            case 0: dll_ = value; break;
            case 1: dlm_ = value; break;
            case 3: lcr_ = value; break;
            case 5: psd_ = value; break;
            default:
                    cvm::log(cvm::ERROR, "uart writing addr 0x{:x}\n", addr);
        }
    }
}
