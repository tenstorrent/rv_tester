#include "post_si_pcietc_sequence.hpp"


REGISTRY_register(post_si_pcietc_sequence, OVERLAY_DRIVER, cvm::registry::all);

DEFINE_bool(post_si_pcietc_xtor_en, false, "Enable post_si_pcietc_sequence tick");
DEFINE_double(post_si_pcietc_delay_multiplier, 10, "Delay multiplier for us and ms");
DEFINE_int32(post_si_pcietc_tick_interval, 121, "Interval between ticks in ns");
DEFINE_string(post_si_pcietc_ee0_file, "", "Sequence file for post_si_pcietc_ee0");
DEFINE_string(post_si_pcietc_ee1_file, "", "Sequence file for post_si_pcietc_ee1");
DEFINE_string(post_si_pcietc_ee2_file, "", "Sequence file for post_si_pcietc_ee2");
DEFINE_string(post_si_pcietc_ee3_file, "", "Sequence file for post_si_pcietc_ee3");

post_si_pcietc_sequence::post_si_pcietc_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {
    // Scope
    cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

    // Get post_si_pcietc_helper location (assuming it's in SYSMOD.TRICKBOX)
    helper_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);

    // Connect to receive signals from post_si_pcietc_helper
    cvm::registry::messenger.procedure<post_si_pcietc_helper_write_rpc>(
        loc_,
        [this](const post_si_pcietc_helper_rpc_data_t &i) { this->handle_write(i); });
    cvm::registry::messenger.procedure<post_si_pcietc_helper_read_rpc>(
        loc_,
        [this](post_si_pcietc_helper_rpc_data_t &i) { this->handle_read(i); });

    // Only create the objects if the flag is enabled; otherwise, use pointers and leave them null
    if (FLAGS_post_si_pcietc_xtor_en) {
        global_node_ = std::make_unique<post_si_pcietc_global>();
        completer_engine_ = std::make_unique<post_si_pcietc_ce>();
        for (int i = 0; i < 4; i++) {
            execution_engine_[i] = std::make_unique<post_si_pcietc_ee>(i);
        }
    }
}

post_si_pcietc_sequence::~post_si_pcietc_sequence() {
}

void post_si_pcietc_sequence::handle_write(const post_si_pcietc_helper_rpc_data_t &data) {

    if (FLAGS_post_si_pcietc_xtor_en) {
        cvm::log(cvm::MEDIUM, "[post_si_pcietc_sequence] Received Write from post_si_pcietc_helper: addr={:#x}, offset={:#x}, data={:#x}\n", data.addr, data.offset, data.data[0]);

        // Possible Memory Mapped Register access from helper
        if (global_node_->handle_write(data)) {
            return;
        }

        if (completer_engine_->handle_write(data)) {
            return;
        }

        for (int i = 0; i < 4; i++) {
            if (execution_engine_[i]->handle_write(data)) {
                return;
            }
        }
    }
    cvm::log(cvm::ERROR, "[post_si_pcietc_sequence] Unsupported write: addr={:#x} length={:d} offset={:#x}\n", data.addr, data.length, data.offset);
}

void post_si_pcietc_sequence::handle_read(post_si_pcietc_helper_rpc_data_t& data) {

    if (FLAGS_post_si_pcietc_xtor_en) {
        cvm::log(cvm::MEDIUM, "[post_si_pcietc_sequence] Received Read from post_si_pcietc_helper: addr={:#x}, offset={:#x}, data={:#x}\n", data.addr, data.offset, data.data[0]);

        if (global_node_->handle_read(data)) {
            return;
        }

        if (completer_engine_->handle_read(data)) {
            return;
        }

        for (int i = 0; i < 4; i++) {
            if (execution_engine_[i]->handle_read(data)) {
                return;
            }
        }
    }
    cvm::log(cvm::ERROR, "[post_si_pcietc_sequence] Unsupported read: addr={:#x} length={:d} offset={:#x}\n", data.addr, data.length, data.offset);
}
