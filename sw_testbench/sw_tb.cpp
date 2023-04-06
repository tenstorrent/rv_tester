#include "cosim_transactions.hpp"
#include "rv_tester.hpp"

int main(int argc, char** argv) {
    // Setup
    rv_tester_parse_flags();
    rv_tester_parse_memmap();
    rv_tester_reset_registry();
    rv_tester_flush_callbacks();

    cosim_transactions::m_rvfi message;
    cosim_transactions_message(message);
}