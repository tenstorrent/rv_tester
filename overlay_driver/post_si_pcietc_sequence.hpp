#pragma once
#include <iostream>
#include <string>
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "sysmod/sysmod_plusargs.h"
#include "post_si_pcietc_defs.hpp"
#include "post_si_pcietc_global.hpp"
#include "post_si_pcietc_ce.hpp"
#include "post_si_pcietc_ee.hpp"


// CVM procedure call declaration
CVM_MESSENGER_procedure_call(post_si_pcietc_helper_write_rpc, void (const post_si_pcietc_helper_rpc_data_t &));
CVM_MESSENGER_procedure_call(post_si_pcietc_helper_read_rpc, void (post_si_pcietc_helper_rpc_data_t &));

class post_si_pcietc_sequence {

public:
    post_si_pcietc_sequence(cvm::topology::loc_t loc, unsigned id);
    ~post_si_pcietc_sequence();

    void set_scope(svScope s) { scope_ = s; }
    uint64_t convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz) {
        uint64_t result=0;

        for (int i = 0; i < static_cast<int>(sz); ++i) {
            result = result | static_cast<uint64_t>(byte_array[shift+i]) << (i*8);
        }

        return result;
    }

    // Handler for post_si_pcietc_helper procedure calls
    void handle_write(const post_si_pcietc_helper_rpc_data_t &data);
    void handle_read(post_si_pcietc_helper_rpc_data_t &data);

private:

    void launch_thread();

    cvm::messenger::task<void> start_processing();

    cvm::topology::loc_t loc_;
    unsigned id_;
    cvm::topology::loc_t helper_loc_;
    svScope scope_;
    std::unique_ptr<post_si_pcietc_global> global_node_;
    std::unique_ptr<post_si_pcietc_ce> completer_engine_;
    std::unique_ptr<post_si_pcietc_ee> execution_engine_[4];
};
