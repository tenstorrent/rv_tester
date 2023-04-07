#include "cosim_transactions.hpp"
#include "rv_tester.hpp"

int main(int argc, char** argv) {
    // Setup
    rv_tester_parse_flags();
    rv_tester_parse_memmap();
    rv_tester_reset_registry();
    rv_tester_flush_callbacks();

    cosim_transactions::m_rvfi message = {
        /*location=*/1,
        /*cycle=*/2,
        /*order=*/3,
        /*insn=*/4,
        /*trap=*/5,
        /*intr=*/6,
        /*cause=*/7,
        /*mode=*/8,
        /*ixl=*/9,
        /*rd_addr=*/10,
        /*rd_wdata=*/11,
        /*pc_rdata=*/12,
        /*pc_wdata=*/13,
        /*mem_addr=*/14,
        /*mem_paddr=*/15,
        /*mem_rmask=*/16,
        /*mem_rdata=*/17,
        /*mem_wmask=*/18,
        /*mem_wdata=*/19
    };
    cosim_transactions_message(reinterpret_cast<uint8_t*>(&message));
}