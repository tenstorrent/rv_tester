#include "post_si_pcietc_ee.hpp"
#include <fstream>
#include <sstream>
#include "overlay_driver.hpp"

DEFINE_int32(post_si_pcietc_max_axi_req, 48, "Maximum number of AXI requests in a tick");
static std::mutex axi_req_cnt_lock;
static int axi_write_req_cnt = 0;
static int axi_read_req_cnt = 0;

// Constructor
post_si_pcietc_ee::post_si_pcietc_ee(int index)
    : reg_ee_cfg_(this), reg_ee_ctl_(this), reg_ee_sts_(this), reg_ee_err_sts_(this),
      reg_ee_fail_addr_lo_(this), reg_ee_fail_addr_hi_(this), reg_ee_fail_exp_(this), reg_ee_fail_act_(this), reg_ee_fail_instr_(this),
      reg_ee_dbg0_(this), reg_ee_dbg1_(this), reg_ee_loop_cnt_(this),
      reg_ee_cdata0_(this, 0x1a2b3c4d, "EE_CDATA0"), reg_ee_cdata1_(this, 0xffffffff, "EE_CDATA1"),
      reg_ee_cdata2_(this, 0x5a5a5a5a, "EE_CDATA2"), reg_ee_cdata3_(this, 0xfeedf00d, "EE_CDATA3") {
    init_register_map();
    ee_index_ = index;
    ib_mem_ = mem_manager();

    std::string file_paths[4] = {FLAGS_post_si_pcietc_ee0_file, FLAGS_post_si_pcietc_ee1_file, FLAGS_post_si_pcietc_ee2_file, FLAGS_post_si_pcietc_ee3_file};
    if (!file_paths[index].empty()) {
        load_instructions(file_paths[index]);
    }
    tick_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.OVERLAY_DRIVER", 0);
    axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST",0);
    r_channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
    wresp_channel = cvm::registry::messenger.channel<axi::b_t>(axi_mst_loc_l);

    if (FLAGS_post_si_pcietc_xtor_en) {
        auto *task = +[] (post_si_pcietc_ee* m) -> cvm::messenger::task<void> {
            co_await m->start_engine();
            co_return;
        };
        cvm::registry::messenger.fork(task, this);

        auto *wr_ack_task = +[] (post_si_pcietc_ee* m) -> cvm::messenger::task<void> {
            co_await m->wr_ack();
            co_return;
        };
        cvm::registry::messenger.fork(wr_ack_task, this);

        auto *rd_cpl_task = +[] (post_si_pcietc_ee* m) -> cvm::messenger::task<void> {
            co_await m->rd_cpl_task();
            co_return;
        };
        cvm::registry::messenger.fork(rd_cpl_task, this);
    }
}

void post_si_pcietc_ee::load_instructions(const std::string& file_path) {
    // Open the file for reading
    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Loading instructions from {}\n", ee_index_, file_path);
    std::ifstream infile(file_path);
    if (!infile.is_open()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Failed to open instruction file: {}\n", ee_index_, file_path);
        return;
    }

    std::string line;
    uint32_t idx = 0;
    while (std::getline(infile, line)) {
        // Trim leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] line: {}\n", ee_index_, line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || (line.size() > 1 && line[0] == '/' && line[1] == '/'))
            continue;

        std::istringstream iss(line);
        uint32_t dword[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; ++i) {
            std::string token;
            if (!(iss >> token)) {
                break;
            }
            // Only accept hex: abcdabcd or 0xabcdabcd
            dword[i] = std::stoul(token, nullptr, 16);
        }

        // Write the 4 dwords into memory at the appropriate offset
        // Each instruction is 16 bytes (4 dwords)
        uint32_t offset = idx * 16;
        std::vector<uint8_t> data(16);
        for (int i = 0; i < 4; ++i) {
            data[i*4 + 0] = (dword[i] >> 0) & 0xFF;
            data[i*4 + 1] = (dword[i] >> 8) & 0xFF;
            data[i*4 + 2] = (dword[i] >> 16) & 0xFF;
            data[i*4 + 3] = (dword[i] >> 24) & 0xFF;
        }
        ib_mem_.write(offset, data);
        ++idx;
    }
    infile.close();
    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Loaded {} instructions from {}\n", ee_index_, idx, file_path);
}

cvm::messenger::task<void> post_si_pcietc_ee::get_axi_req_token(bool write_req) {
    while (1) {
        axi_req_cnt_lock.lock();
        if (write_req) {
            if (axi_write_req_cnt == FLAGS_post_si_pcietc_max_axi_req) {
                cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Warning: Max AXI write requests reached: {}. Waiting for AXI requests to complete.\n", ee_index_, axi_write_req_cnt);
                axi_req_cnt_lock.unlock();
                co_await tick();
                continue;
            }
            axi_write_req_cnt++;
        } else {
            if (axi_read_req_cnt == FLAGS_post_si_pcietc_max_axi_req) {
                cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Warning: Max AXI read requests reached: {}. Waiting for AXI requests to complete.\n", ee_index_, axi_read_req_cnt);
                axi_req_cnt_lock.unlock();
                co_await tick();
                continue;
            }
            axi_read_req_cnt++;
        }
        axi_req_cnt_lock.unlock();
        break;
    }
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::release_axi_req_token(bool write_req) {
    axi_req_cnt_lock.lock();
    if (write_req) {
        axi_write_req_cnt--;
    } else {
        axi_read_req_cnt--;
    }
    axi_req_cnt_lock.unlock();
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::rd_cpl_task() {
    while (1) {
        auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(axi_mst_loc_l);
        cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] Received read response id: {} \n", ee_index_, resp.id);
        // Check if r.id is a key of axi_read_tracker_ and delete it
        auto it = axi_read_tracker_.find(resp.id);
        if (it != axi_read_tracker_.end()) {
            std::unique_ptr<ee_instruction_base_t> instr;
            uint32_t curr_instr_idx = it->second & 0x00ffffff;
            uint32_t burst = it->second >> 32;

            bool success = co_await engine_get_instruction(curr_instr_idx, instr);
            if (!success) {
                cvm::log(cvm::ERROR, "[post_si_pcietc_ee{}] Error: Unable to fetch instruction number {} to check read data size\n", ee_index_, curr_instr_idx);
                co_return;
            }
            ee_memory_rw_instruction_t* mem_instr = static_cast<ee_memory_rw_instruction_t*>(instr.get());

            // If brust is non-zero then we are doing multiple MAX_BEAT_SIZE beats
            uint64_t curr_addr = mem_instr->get_address() + (burst * MAX_BEAT_SIZE);
            size_t addr_offset = curr_addr % MAX_BEAT_SIZE;
            size_t expected_size = (mem_instr->get_size() < MAX_BEAT_SIZE) ? mem_instr->get_size() : MAX_BEAT_SIZE;

            std::vector<uint32_t> extracted_data = post_si_pcietc::pack_data_to_u32(curr_addr, std::vector<uint8_t>(resp.data.begin() + addr_offset, resp.data.end()));

            std::vector<uint8_t> expected_data_u8 = post_si_pcietc::generate_pattern(mem_instr->get_data_pattern(), curr_addr, expected_size, reg_ee_loop_cnt_.read(), reg_ee_cdata0_.read(), reg_ee_cdata1_.read(), reg_ee_cdata2_.read(), reg_ee_cdata3_.read());
            std::vector<uint32_t> expected_data = post_si_pcietc::pack_data_to_u32(curr_addr, expected_data_u8);

            // convert to dword aligned address
            curr_addr &= ~0x3ULL;
            for (size_t i = 0; i < expected_data.size(); i++) {
                if (i >= extracted_data.size()) {
                    cvm::log(cvm::ERROR, "[post_si_pcietc_ee{}] Error: Expected data size is greater than extracted data size for instruction number: {}. Expected size: {}, Extracted size: {}\n", ee_index_, curr_instr_idx, expected_data.size(), extracted_data.size());
                    engine_log_error(ee_instr_error_t::DATA_SIZE_MISMATCH_ERROR);
                    break;
                }
                if (extracted_data[i] != expected_data[i]) {
                    engine_log_failure(curr_instr_idx, curr_addr, expected_data[i], extracted_data[i]);
                }
                curr_addr += 4;
            }
            axi_read_tracker_.erase(it);
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Deleted rd_id: {} for instruction number: {}, burst: {}. Pending rd_id size: {}\n", ee_index_, resp.id, curr_instr_idx, burst, axi_read_tracker_.size());
            co_await release_axi_req_token(false);
        }
    }
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_memory_read_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    ee_memory_rw_instruction_t* mem_instr = static_cast<ee_memory_rw_instruction_t*>(instr.get());

    // Validate size and address using the new constraint
    if (!mem_instr->is_size_valid()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_memory_read_instruction: Invalid size: size={:#x}\n",
                 ee_index_, mem_instr->get_size());
        engine_log_error(ee_instr_error_t::INVALID_SIZE_ERROR);
        co_return;
    }
    if (!mem_instr->is_address_valid()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_memory_read_instruction: Invalid address: addr={:#x} for size={:#x}\n",
                 ee_index_, mem_instr->get_address(), mem_instr->get_size());
        engine_log_error(ee_instr_error_t::ADDRESS_UNALIGNED_ERROR);
        co_return;
    }

    uint64_t curr_addr = mem_instr->get_address();
    size_t length = mem_instr->get_size();

    uint32_t axi_size = (length <= MAX_BEAT_SIZE) ? length : MAX_BEAT_SIZE;
    uint32_t burst_beats = (length <= MAX_BEAT_SIZE) ? 1 : (length / MAX_BEAT_SIZE);

    while(1) {
        auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_l);
        if(lock) { break; }
        co_await tick();
    }

    // Create AXI AR transaction for each beat
    for (size_t burst = 0; burst < burst_beats; burst++) {
        unsigned id;
        axi::a_no_id_t ar_txn;
        ar_txn.w = false;
        ar_txn.addr = curr_addr;  // Always send aligned address
        ar_txn.size = log2(axi_size);
        ar_txn.len = 0; // AXI len is always 0
        ar_txn.burst = axi::burst_t(1); // INCR burst type
        ar_txn.lock = 0;
        ar_txn.cache = axi::cache_mem_attr_t(0);
        ar_txn.prot = 2;
        ar_txn.qos = 0;
        ar_txn.region = 0;
        ar_txn.atop = 0;
        ar_txn.user = 0;
        ar_txn.seqid = 0;  // Sequence ID for scratchpad memory
        ar_txn.is_manual_id = false;
        ar_txn.manual_id = 0;

        co_await get_axi_req_token(false);

        // Send AXI AR transaction
        do {
            if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_l, ar_txn, id)) {
                break;
            }
            co_await tick();
        } while (true);

        // Track this read request with original unaligned address
        axi_read_tracker_[id] = (((uint64_t)burst)<<32) + instr_idx_;

        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Sent AR transaction id: {} for instruction: {} addr: {:#x} length: {} unrolled beat: {}\n",
                    ee_index_, id, instr_idx_, curr_addr, axi_size, burst);

        curr_addr += axi_size;
    }
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::wr_ack() {
    while (1) {
        axi::b_t wresp = co_await cvm::registry::messenger.wait<axi::b_t>(wresp_channel);
        cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] Received write response id: {} resp: {} \n", ee_index_, wresp.id, wresp.resp);
        // Check if resp.id is a key of pending_wr_id_ and delete it
        auto it = pending_wr_id_.find(wresp.id);
        if (it != pending_wr_id_.end()) {
            if (wresp.resp != axi::RESP_OKAY) {
                cvm::log(cvm::ERROR, "[post_si_pcietc_ee{}] Error: Bad write response id: {} resp: {} for instruction number: {} \n", ee_index_, wresp.id, wresp.resp, it->second);
                co_return;
            }

            pending_wr_id_.erase(it);
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Deleted wr_id: {} for instruction number: {}. Pending wr_id size: {}\n", ee_index_, wresp.id, it->second & 0xffffffff, pending_wr_id_.size());
            co_await release_axi_req_token(true);
        }
    }
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_memory_write_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    ee_memory_rw_instruction_t* mem_instr = static_cast<ee_memory_rw_instruction_t*>(instr.get());

    // Validate size and address using the new constraint
    if (!mem_instr->is_size_valid()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_memory_write_instruction: Invalid size: size={:#x}\n",
                 ee_index_, mem_instr->get_size());
        engine_log_error(ee_instr_error_t::INVALID_SIZE_ERROR);
        co_return;
    }

    if (!mem_instr->is_address_valid()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_memory_write_instruction: Invalid address: addr={:#x} for size={:#x}\n",
                 ee_index_, mem_instr->get_address(), mem_instr->get_size());
        engine_log_error(ee_instr_error_t::ADDRESS_UNALIGNED_ERROR);
        co_return;
    }

    uint64_t curr_addr = mem_instr->get_address();
    size_t length = mem_instr->get_size();

    uint32_t axi_size = (length <= MAX_BEAT_SIZE) ? length : MAX_BEAT_SIZE;
    uint32_t burst_beats = (length <= MAX_BEAT_SIZE) ? 1 : (length / MAX_BEAT_SIZE);

    // Generate data - address is aligned to size, but may need beat alignment
    std::vector<uint8_t> data = post_si_pcietc::generate_pattern(mem_instr->get_data_pattern(), curr_addr, length,
                                                               reg_ee_loop_cnt_.read(), reg_ee_cdata0_.read(), reg_ee_cdata1_.read(),
                                                               reg_ee_cdata2_.read(), reg_ee_cdata3_.read());

    // Create strobe vector for original data (all valid)
    std::vector<bool> strb(length, true);

    // Calculate beat-aligned address and offset
    size_t addr_offset = curr_addr & (MAX_BEAT_SIZE - 1);

    // Pad beginning of data/strobe for beat alignment
    if (addr_offset != 0) {
        data.insert(data.begin(), addr_offset, 0);
        strb.insert(strb.begin(), addr_offset, false);
    }

    // Pad end of data/strobe to beat_size boundary
    if (data.size() % MAX_BEAT_SIZE != 0) {
        size_t end_pad = MAX_BEAT_SIZE - (data.size() % MAX_BEAT_SIZE);
        data.insert(data.end(), end_pad, 0);
        strb.insert(strb.end(), end_pad, false);
    }

    while(1) {
        auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_l);
        if(lock) { break; }
        co_await tick();
    }

    // Overlay port limitation that length is limited to 1. Repeat steps for burst_beats
    for (size_t burst = 0; burst < burst_beats; burst++) {
        // Prepare single AW transaction with original unaligned address
        axi::a_no_id_t aw_txn;
        aw_txn.w = true;
        aw_txn.addr = curr_addr;
        aw_txn.size = log2(axi_size);
        aw_txn.len = 0;
        aw_txn.burst = axi::burst_t(1); // INCR
        aw_txn.lock = 0;
        aw_txn.cache = axi::cache_mem_attr_t(0);
        aw_txn.prot = 2;
        aw_txn.qos = 0;
        aw_txn.region = 0;
        aw_txn.atop = 0;
        aw_txn.user = 0;
        aw_txn.seqid = 0;
        aw_txn.is_manual_id = false;
        aw_txn.manual_id = 0;

        unsigned id = 0;

        co_await get_axi_req_token(true);

        // Send AW transaction
        do {
            if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_l, aw_txn, id)) {
                break;
            }
            co_await tick();
        } while (true);

        pending_wr_id_[id] = (((uint64_t)burst)<<32) + instr_idx_;

        // Send W data beats - simple extraction from padded data and strobe vectors
        size_t beat_offset = burst * MAX_BEAT_SIZE;

        // Extract beat data and strobe directly from padded vectors
        std::vector<uint8_t> beat_data(data.begin() + beat_offset, data.begin() + beat_offset + MAX_BEAT_SIZE);
        std::vector<bool> beat_strb(strb.begin() + beat_offset, strb.begin() + beat_offset + MAX_BEAT_SIZE);

        cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(axi_mst_loc_l, axi::w_t{beat_data, beat_strb, true});

        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Sent AW and W transactions id: {} for instruction number: {} addr: {:#x} length: {} for unrolled beat: {}\n",
                ee_index_, id, instr_idx_, curr_addr, axi_size, burst);

        curr_addr += axi_size;
    }
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_msi_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    // Follow the write instruction and do the MSI write.
    // Address comes from the instruction, data is from the instruction, not pattern based.
    // Size is always 4 bytes.

    // Downcast to the correct instruction type if needed
    auto* msi_instr = dynamic_cast<ee_msi_instruction_t*>(instr.get());
    if (!msi_instr) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_msi_instruction: Bad instruction type\n", ee_index_);
        engine_log_error(ee_instr_error_t::INSTRUCTION_ERROR);
        co_return;
    }

    if (!msi_instr->is_address_valid()) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_msi_instruction: Invalid address\n", ee_index_);
        engine_log_error(ee_instr_error_t::ADDRESS_UNALIGNED_ERROR);
        co_return;
    }

    uint64_t addr = msi_instr->get_address();
    uint32_t msi_data = msi_instr->get_data();

    // Prepare W data (4 bytes)
    std::vector<uint8_t> data(4);
    data[0] = static_cast<uint8_t>(msi_data & 0xFF);
    data[1] = static_cast<uint8_t>((msi_data >> 8) & 0xFF);
    data[2] = static_cast<uint8_t>((msi_data >> 16) & 0xFF);
    data[3] = static_cast<uint8_t>((msi_data >> 24) & 0xFF);

    std::vector<bool> strb(4, true); // all bytes valid

    // Calculate beat-aligned address and offset
    uint64_t beat_aligned_addr = addr & ~(MAX_BEAT_SIZE - 1);  // Align to beat_size boundary
    size_t addr_offset = addr - beat_aligned_addr;         // Offset within the beat

    // Pad beginning of data/strobe for beat alignment
    if (addr_offset != 0) {
        data.insert(data.begin(), addr_offset, 0);
        strb.insert(strb.begin(), addr_offset, false);
    }

    // Pad end of data/strobe to beat_size boundary
    if (data.size() % MAX_BEAT_SIZE != 0) {
        size_t end_pad = MAX_BEAT_SIZE - data.size();
        data.insert(data.end(), end_pad, 0);
        strb.insert(strb.end(), end_pad, false);
    }

    co_await get_axi_req_token(true);

    while(1) {
        auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_l);
        if(lock) { break; }
        co_await tick();
    }

    // Prepare AW transaction (address write)
    axi::a_no_id_t aw_txn;
    aw_txn.addr = addr;
    aw_txn.size = 2; // 2^2 = 4 bytes
    aw_txn.len = 0;  // single beat
    aw_txn.burst = axi::burst_t(1); // INCR
    aw_txn.lock = 0;
    aw_txn.cache = axi::cache_mem_attr_t(0);
    aw_txn.prot = 2;
    aw_txn.qos = 0;
    aw_txn.region = 0;
    aw_txn.atop = 0;
    aw_txn.user = 0;
    aw_txn.seqid = 0;
    aw_txn.is_manual_id = false;
    aw_txn.manual_id = 0;

    unsigned int id = 0;

    // Send AW transaction
    do {
        if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_l, aw_txn, id)) {
            break;
        }
        co_await tick();
    } while (true);

    pending_wr_id_[id] = instr_idx_;

    // Send W data
    cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(
        axi_mst_loc_l,
        axi::w_t{data, strb, true}
    );

    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Sent MSI AW transaction id: {} addr: {:#x} data: {:#x}\n", ee_index_, id, addr, msi_data);
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_fence_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    // Fence instruction performs synchronization operations based on fence type

    // Downcast to the correct instruction type if needed
    auto* fence_instr = dynamic_cast<ee_fence_instruction_t*>(instr.get());
    if (!fence_instr) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_fence_instruction: Bad instruction type\n", ee_index_);
        engine_log_error(ee_instr_error_t::INSTRUCTION_ERROR);
        co_return;
    }

    uint32_t fence_type = fence_instr->get_fence_type();

    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Executing FENCE instruction with type: {:#x}\n", ee_index_, fence_type);

    // Different fence types can implement different synchronization behaviors
    switch (fence_type) {
        case static_cast<uint32_t>(ee_fence_type_t::NOP):  // Standard fence - ensure memory ordering
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] FENCE:NOP completed\n", ee_index_);
            break;
        case static_cast<uint32_t>(ee_fence_type_t::ENGINE_IDLE):  // I/O fence - ensure I/O ordering
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] FENCE: ENGINE_IDLE completed\n", ee_index_);
            // Fence operation: ensure all previous memory operations are completed
            // Wait for all pending read and write operations to complete
            if (pending_wr_id_.size() || axi_read_tracker_.size()) {
                cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] FENCE: Waiting for pending operations - wr_id size: {} rd_id size: {}\n",
                        ee_index_, pending_wr_id_.size(), axi_read_tracker_.size());
                while (pending_wr_id_.size() > 0 || axi_read_tracker_.size() > 0) {
                    co_await tick();
                }
            }
            break;
        default:   // Unknown fence types
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] FENCE: Unknown fence type {:#x} completed\n", ee_index_, fence_type);
            break;
    }

    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] FENCE instruction completed\n", ee_index_);
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_loop_control_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    // Loop Control instruction manages loop execution flow

    // Downcast to the correct instruction type if needed
    auto* loop_instr = dynamic_cast<ee_loop_control_instruction_t*>(instr.get());
    if (!loop_instr) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] handle_loop_control_instruction: Bad instruction type\n", ee_index_);
        engine_log_error(ee_instr_error_t::INSTRUCTION_ERROR);
        co_return;
    }

    bool loop_end = loop_instr->get_loop_end();

    if (loop_end) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] LOOP_CONTROL: Completed Loop iteration count: {} of {}\n", ee_index_, reg_ee_loop_cnt_.read(), tgt_loop_count_);

        if ((reg_ee_loop_cnt_.reg_data.fields.loop_cnt + 1) < tgt_loop_count_) {
            // Even though we assign to loop start, we dont go back to loop start because the caller increments the instruction index
            reg_ee_sts_.reg_data.fields.instruction = loop_start_idx_;
            reg_ee_loop_cnt_.reg_data.fields.loop_cnt += 1;
        }
    } else {
        tgt_loop_count_ = loop_instr->get_loop_count();
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Executing LOOP_CONTROL instruction - Loop Start with Count: {} at instruction {}\n",
             ee_index_, tgt_loop_count_, instr_idx_);

        // Loop control bit not set - could be used for other control operations
        loop_start_idx_ = instr_idx_;
        reg_ee_loop_cnt_.reg_data.fields.loop_cnt = 0;
    }

    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::handle_end_instruction(std::unique_ptr<ee_instruction_base_t>& ) {
    if (pending_wr_id_.size() || axi_read_tracker_.size()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] End instruction: Pending wr_id size: {} pending rd_id size: {}\n", ee_index_, pending_wr_id_.size(), axi_read_tracker_.size());
        while (pending_wr_id_.size() > 0 || axi_read_tracker_.size() > 0) {
            co_await tick();
        }
    }

    if (static_cast<ee_mode_t>(reg_ee_cfg_.reg_data.fields.mode) == ee_mode_t::INF_REPEAT) {
        if (engine_stop_req_) {
            engine_state_ = ee_state_t::EE_STOPPED;
            engine_stop_req_ = false;
        } else {
            engine_state_ = ee_state_t::EE_START_REQ;
            engine_start_req_ = true;
        }
    } else {
        engine_state_ = ee_state_t::EE_STOPPED;
    }
    co_return;
}

void post_si_pcietc_ee::engine_log_error(ee_instr_error_t error) {
    engine_state_ = ee_state_t::EE_ERROR;
    if (!reg_ee_sts_.reg_data.fields.error_detected) {
        reg_ee_sts_.reg_data.fields.error_detected = 1;
        reg_ee_err_sts_.reg_data.fields.reason = static_cast<uint32_t>(error);
    }
}

cvm::messenger::task<bool> post_si_pcietc_ee::engine_get_instruction(unsigned int instr_idx, std::unique_ptr<ee_instruction_base_t>& instr) {
    // Read 4 DWORDs (16 bytes) from instruction buffer at instr_idx * 16
    std::array<uint32_t, 4> dwords = {0, 0, 0, 0};
    std::vector<uint8_t> instr_bytes = ib_mem_.read(instr_idx * 16, 16);
    for (int i = 0; i < 4; ++i) {
        dwords[i] = 0;
        for (int j = 0; j < 4; ++j) {
            dwords[i] |= static_cast<uint32_t>(instr_bytes[i * 4 + j]) << (8 * j);
        }
    }
    // Convert to instruction class
    instr = post_si_pcietc_ee_instr::create_instruction_from_dwords(dwords);
    if (!instr) {
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] engine_get_instruction: Bad instruction found at instr_idx={}\n", ee_index_, instr_idx);
        engine_log_error(ee_instr_error_t::INSTRUCTION_ERROR);
        co_return false;
    }

    // Processing new instruction
    cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Processing instr_idx={} : {}\n", ee_index_, instr_idx, instr->to_string());

    co_return true;
}

cvm::messenger::task<bool> post_si_pcietc_ee::engine_delay_instruction(std::unique_ptr<ee_instruction_base_t>& instr) {
    if (instr->get_delay_value() != 0) {
        int delay_tgt_ticks = static_cast<int>(instr->get_delay_value());
        switch (instr->get_delay_granularity()) {
            case ee_delay_gran_t::MILLISECOND:
                delay_tgt_ticks *= FLAGS_post_si_pcietc_delay_multiplier;
                // intentional fall through
            case ee_delay_gran_t::MICROSECOND:
                delay_tgt_ticks *= FLAGS_post_si_pcietc_delay_multiplier;
                // intentional fall through
            case ee_delay_gran_t::NANOSECOND:
                break;

            default:
                cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] engine_get_instruction: Invalid delay granularity={}\n", ee_index_, static_cast<uint32_t>(instr->get_delay_granularity()));
                engine_log_error(ee_instr_error_t::DELAY_ERROR);
                co_return false;
        }
        delay_tgt_ticks /= FLAGS_post_si_pcietc_tick_interval;
        cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Delaying instruction for {} ticks\n", ee_index_, delay_tgt_ticks);
        co_await tick(delay_tgt_ticks);
    }

    co_return true;
}

cvm::messenger::task<void> post_si_pcietc_ee::engine_process_instruction(bool &internal_instruction) {
    instr_idx_ = static_cast<uint32_t>(reg_ee_sts_.reg_data.fields.instruction);
    std::unique_ptr<ee_instruction_base_t> instr;

    bool success = co_await engine_get_instruction(instr_idx_, instr);
    if (!success) {
        co_return;
    }

    bool delay_success = co_await engine_delay_instruction(instr);
    if (!delay_success) {
        co_return;
    }

    switch (instr->get_instruction_type()) {
        case ee_instr_type_t::MEMORY_READ: {
            co_await handle_memory_read_instruction(instr);
            internal_instruction = false;
            break;
        }
        case ee_instr_type_t::MEMORY_WRITE: {
            co_await handle_memory_write_instruction(instr);
            internal_instruction = false;
            break;
        }
        case ee_instr_type_t::MSI: {
            co_await handle_msi_instruction(instr);
            internal_instruction = false;
            break;
        }
        case ee_instr_type_t::LOOP_CONTROL: {
            co_await handle_loop_control_instruction(instr);
            internal_instruction = true;
            break;
        }
        case ee_instr_type_t::FENCE: {
            co_await handle_fence_instruction(instr);
            internal_instruction = true;
            break;
        }
        case ee_instr_type_t::END: {
            co_await handle_end_instruction(instr);
            // SPECIAL CASE: END instruction is handled in the handle_end function and we directly return
            internal_instruction = true;
            co_return;
        }
    }

    reg_ee_sts_.reg_data.fields.instruction++;
    co_return;
}

cvm::messenger::task<void> post_si_pcietc_ee::start_engine() {
    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] Starting engine\n", ee_index_);
    bool internal_instruction = false;
    while (1) {
        if (engine_state_ == ee_state_t::EE_RUNNING && internal_instruction) {
            // When dealing with internal instructions, we don't need to wait for a tick, process next instruction immediately
        } else {
            co_await tick();
        }

        if (engine_start_req_) {
            engine_state_ = ee_state_t::EE_RUNNING;
            engine_start_req_ = false;
            reg_ee_sts_.reg_data.fields.instruction = 0;
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] engine started\n", ee_index_);
        }

        if (engine_state_ == ee_state_t::EE_RUNNING) {
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] engine running next instruction\n", ee_index_);
            co_await engine_process_instruction(internal_instruction);
        }

        if (engine_state_ == ee_state_t::EE_ERROR || engine_state_ == ee_state_t::EE_STOPPED) {
            engine_state_ = ee_state_t::EE_IDLE;
            reg_ee_sts_.reg_data.fields.stopped = 1;
            cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] engine stopped\n", ee_index_);
        }
    }
    co_return;
}

void post_si_pcietc_ee::engine_log_failure(unsigned int idx, uint64_t addr, uint32_t expected, uint32_t actual) {
    cvm::log(cvm::NONE, "[post_si_pcietc_ee{}] Failure: Data mismatch: at address {:#x} Expected: {:#x}, Actual: {:#x} for instruction number: {}\n", ee_index_, addr, expected, actual, idx);
    if (reg_ee_sts_.reg_data.fields.failure_detected == 1) {
        reg_ee_sts_.reg_data.fields.multiple_fail = 1;
    } else {
        reg_ee_sts_.reg_data.fields.failure_detected = 1;
        reg_ee_fail_addr_lo_.reg_data.fields.addr_lo = addr & 0xffffffff;
        reg_ee_fail_addr_hi_.reg_data.fields.addr_hi = (addr >> 32) & 0xffffffff;
        reg_ee_fail_exp_.reg_data.fields.data = expected;
        reg_ee_fail_act_.reg_data.fields.data = actual;
        reg_ee_fail_instr_.reg_data.fields.index = static_cast<uint16_t>(idx);
    }
}

cvm::messenger::task<void> post_si_pcietc_ee::tick(int ticks) {
    uint64_t tgt_tick = cnt_tick + static_cast<uint64_t>(ticks);
    while (cnt_tick < tgt_tick) {
        co_await cvm::registry::messenger.wait<rv_tester_transactions::overlay_driver::m_overlay_driver_tick<>>(tick_loc_);
        cnt_tick = cnt_tick + 1;
    }
    // cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] tick: cnt_tick={}\n", ee_index_, cnt_tick);
    co_return;
}

// Destructor
post_si_pcietc_ee::~post_si_pcietc_ee() {
    // No special cleanup needed
}

// Register read function - takes offset and returns data
void post_si_pcietc_ee::reg_read(uint32_t offset, uint32_t &data) {
    data = 0;

    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] reg_read: found offset={:#x} in map: {}\n", ee_index_, offset, it->second->get_name());
        data = it->second->read();
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] reg_read: offset={:#x} not found in register map\n", ee_index_, offset);
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] register_map_: offset={:#x}, name={}\n", ee_index_, reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, data remains 0
}

// Register write function - takes offset and data
void post_si_pcietc_ee::reg_write(uint32_t offset, uint32_t data) {
    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] reg_write: offset={:#x} found in map: {}\n", ee_index_, offset, it->second->get_name());
        it->second->write(data);
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] reg_write: offset={:#x} not found in register map\n", ee_index_, offset);
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] register_map_: offset={:#x}, name={}\n", ee_index_, reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, silently ignore
}

// Memory read function - reads data from memory address
std::vector<uint8_t> post_si_pcietc_ee::memory_read(uint32_t offset, size_t length) {
    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] memory_read: offset={:#x} length={}\n", ee_index_, offset, length);
    return ib_mem_.read(offset, length);
}

// Memory write function - writes data to memory address
void post_si_pcietc_ee::memory_write(uint32_t offset, size_t length, const std::vector<uint8_t>& data) {
    cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] memory_write: offset={:#x} data length={}\n", ee_index_, offset, length);
    ib_mem_.write(offset, data);
}

bool post_si_pcietc_ee::handle_read(post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;
    uint32_t ee_register_offsets[] = {
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + (2 * static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE)),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + (3 * static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE))
    };
    if (is_in_range(offset, length, ee_register_offsets[ee_index_], post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_ee{}] handle_read: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", ee_index_, offset, length);
            return false;
        }
        uint32_t read_data = 0;
        uint32_t block_offset = static_cast<uint32_t>(offset - ee_register_offsets[ee_index_]);
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] handle_read: Register: offset={:#x} block_offset={:#x}\n", ee_index_, offset, block_offset);
        reg_read(block_offset, read_data);
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] handle_read: Register: offset={:#x} block_offset={:#x} read_data={:#x}\n", ee_index_, offset, block_offset, read_data);
        SERIALIZE_INT(read_data, 4, data.data);
        return true;
    }

    uint32_t instr_buffer_offsets[] = {
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE0_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE1_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE2_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE3_INSTR_BUFFER)
    };
    if (is_in_range(offset, length, instr_buffer_offsets[ee_index_], post_si_pcietc::size_constants::INSTR_BUFFER_SIZE)) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] handle_read: Instruction Buffer: offset={:#x} length={}\n", ee_index_, offset, length);
        uint32_t instr_buffer_offset = static_cast<uint32_t>(offset - instr_buffer_offsets[ee_index_]);
        data.data = memory_read(instr_buffer_offset, length);
        return true;
    }

    cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] handle_read: offset={:#x} length={} not handled\n", ee_index_, offset, length);
    return false;
}

bool post_si_pcietc_ee::handle_write(const post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;

    uint32_t ee_register_offsets[] = {
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + (2 * static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE)),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE_REGISTERS) + (3 * static_cast<uint32_t>(post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE))
    };
    if (is_in_range(offset, length, ee_register_offsets[ee_index_], post_si_pcietc::size_constants::EE_REGISTER_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_ee{}] handle_write: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", ee_index_, offset, length);
            return false;
        }
        uint32_t write_data = 0;
        DESERIALIZE_INT(data.data, 4, write_data);
        uint32_t block_offset = static_cast<uint32_t>(offset - ee_register_offsets[ee_index_]);
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] handle_write: Register: offset={:#x} block_offset={:#x} write_data={:#x}\n", ee_index_, offset, block_offset, write_data);
        reg_write(block_offset, write_data);
        return true;
    }

    uint32_t instr_buffer_offsets[] = {
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE0_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE1_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE2_INSTR_BUFFER),
        static_cast<uint32_t>(post_si_pcietc::address_region_t::EE3_INSTR_BUFFER)
    };
    if (is_in_range(offset, length, instr_buffer_offsets[ee_index_], post_si_pcietc::size_constants::INSTR_BUFFER_SIZE)) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] handle_write: Instruction Buffer: offset={:#x} length={}\n", ee_index_, offset, length);
        uint32_t instr_buffer_offset = static_cast<uint32_t>(offset - instr_buffer_offsets[ee_index_]);
        memory_write(instr_buffer_offset, length, data.data);
        return true;
    }

    cvm::log(cvm::DEBUG, "[post_si_pcietc_ee{}] handle_write: offset={:#x} length={} not handled\n", ee_index_, offset, length);
    return false;
}

void post_si_pcietc_ee::init_register_map() {
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CFG)]          = &reg_ee_cfg_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CTL)]          = &reg_ee_ctl_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_STS)]          = &reg_ee_sts_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_ERR_STS)]      = &reg_ee_err_sts_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_FAIL_ADDR_LO)] = &reg_ee_fail_addr_lo_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_FAIL_ADDR_HI)] = &reg_ee_fail_addr_hi_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_FAIL_EXP)]     = &reg_ee_fail_exp_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_FAIL_ACT)]     = &reg_ee_fail_act_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_FAIL_INSTR)]   = &reg_ee_fail_instr_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_DBG0)]         = &reg_ee_dbg0_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_DBG1)]         = &reg_ee_dbg1_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_LOOP_CNT)]     = &reg_ee_loop_cnt_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CDATA0)]       = &reg_ee_cdata0_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CDATA1)]       = &reg_ee_cdata1_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CDATA2)]       = &reg_ee_cdata2_;
    register_map_[static_cast<uint32_t>(ee_regs_t::EE_CDATA3)]       = &reg_ee_cdata3_;
}


void ee_ctl_reg_t::write(uint32_t value) {
    reg_data.raw_value = value;

    if (reg_data.fields.reset) {
        // Clear failure and error registers when reset bit is set
        parent_ee_->engine_start_req_ = false;
        parent_ee_->engine_stop_req_ = false;
        parent_ee_->engine_state_ = post_si_pcietc_ee::ee_state_t::EE_IDLE;
        parent_ee_->reg_ee_sts_.reg_data.fields.stopped = 0;
        parent_ee_->reg_ee_sts_.reg_data.fields.error_detected = 0;
        parent_ee_->reg_ee_sts_.reg_data.fields.failure_detected = 0;
        parent_ee_->reg_ee_sts_.reg_data.fields.multiple_fail = 0;
        parent_ee_->reg_ee_sts_.reg_data.fields.instruction = 0;
        parent_ee_->reg_ee_err_sts_.reg_data.fields.reason = 0;
        parent_ee_->reg_ee_fail_addr_lo_.reg_data.fields.addr_lo = 0;
        parent_ee_->reg_ee_fail_addr_hi_.reg_data.fields.addr_hi = 0;
        parent_ee_->reg_ee_fail_exp_.reg_data.fields.data = 0;
        parent_ee_->reg_ee_fail_act_.reg_data.fields.data = 0;
        parent_ee_->reg_ee_fail_instr_.reg_data.fields.index = 0;
        parent_ee_->tgt_loop_count_ = 0;
        parent_ee_->loop_start_idx_ = 0;
        parent_ee_->reg_ee_loop_cnt_.reg_data.fields.loop_cnt = 0;
        parent_ee_->instr_idx_ = 0;
        parent_ee_->axi_read_tracker_.clear();
        parent_ee_->pending_wr_id_.clear();

        reg_data.fields.reset = 0;  // Write-only bit, auto-clear
    }

    if (reg_data.fields.start) {
        // Set started state (clear stopped bit)
        parent_ee_->engine_start_req_ = true;
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] start engine requested\n", parent_ee_->ee_index_);
        reg_data.fields.start = 0;  // Write-only bit, auto-clear
    }

    if (reg_data.fields.stop) {
        // Set stopped state
        parent_ee_->engine_stop_req_ = true;
        cvm::log(cvm::HIGH, "[post_si_pcietc_ee{}] stop engine requested\n", parent_ee_->ee_index_);
        reg_data.fields.stop = 0;  // Write-only bit, auto-clear
    }
}
