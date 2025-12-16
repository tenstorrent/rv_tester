#pragma once

#include <fstream>
#include <fmt/format.h>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <vector>
#include <string>

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "rv_tester_transactions.hpp"

#include "cosim/bridge/bridge.h"
#include "rv_tester/rv_tester_structs.h"

class mcmi {

  template<typename T, typename... Args> void connect(cvm::topology::loc_t loc) {
    cvm::registry::messenger.connect<T>(
      loc,
      [this] (const T& v) {
        if (!terminated_) return this->process(v);
      }
    );
    if constexpr (sizeof...(Args)) {
      connect<Args...>(loc);
    }
  }

  public:

    mcmi(cvm::topology::loc_t loc, unsigned id, std::shared_ptr<bridge> bridge);
    ~mcmi();
    void check();

  private:

    void init();
    void process(const rv_tester_transactions::cosim::m_reset<>& m_reset);
    void process(const rv_tester_transactions::cosim::m_trap<>& m_trap);
    void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert);
    void process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass);
    void process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp);
    void process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict);
    void process(const rv_tester_transactions::cosim::m_mcmi_devict<>& m_mcmi_devict);
    void process(const rv_tester_transactions::cosim::m_mcmi_flush<>& m_mcmi_flush);
    void process(const rv_tester_transactions::cosim::m_mcmi_writeback<>& m_mcmi_writeback);
    void process(const rv_tester_transactions::cosim::m_mcmi_dfetch<>& m_mcmi_dfetch);

    void process(const rv_tester::terminate_called&);
    void process(const rv_tester::terminate_called_mem_checks&);
    void process(const bridge::error_loc &);

    bool patch_access (uint64_t addr);
    bool is_ncio(uint32_t mem_attr);
    bool check_axi_error(uint64_t addr);
    std::bitset<256> stringToBitset(const std::string& hexString);
    std::bitset<256> extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb);
    bool sc_failed(mem_t& write);
    void process_amo(mem_t& read);
    void amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);
    void process_ncio_fetches(const rv_instr_t& instr);

    // Helper functions for m_mcmi_read processing
    void process_read_single_consecutive(mem_t& m, const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read,
                                         const std::bitset<256>& data_vec, uint8_t elemsize, bool cacheable);
    void process_read_split_range(mem_t& m, const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read,
                                  uint64_t start_addr, size_t size, const std::bitset<256>& data_vec,
                                  const std::string& hex_string, uint8_t elemsize, bool cacheable);

    // Utility function to process non-consecutive memory accesses
    // Calls the callback for each consecutive address range
    // Callback signature: void(uint64_t start_addr, size_t size, const std::bitset<256>& data_vec, const std::string& hex_string)
    template<typename Callback>
    void process_split_memory_accesses(uint64_t base_addr, uint64_t mask,
                                       const std::bitset<256>& data_vec,
                                       Callback&& callback);

    // High-level utility function that handles both consecutive and non-consecutive memory accesses
    // If consecutive, calls single_callback once. Otherwise, calls split_callback for each range.
    // single_callback signature: void()
    // split_callback signature: void(uint64_t start_addr, size_t size, const std::bitset<256>& data_vec, const std::string& hex_string)
    template<typename SingleCallback, typename SplitCallback>
    void process_memory_access(uint64_t base_addr, uint64_t mask,
                               const std::bitset<256>& data_vec,
                               SingleCallback&& single_callback,
                               SplitCallback&& split_callback);

    template <typename T>
    void amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);

  private:

    cvm::file_logger log;
    [[maybe_unused]] cvm::topology::loc_t loc_;
    [[maybe_unused]] unsigned id_;

    std::shared_ptr<bridge> bridge_;

    bool in_reset_ = true;
    bool terminated_ = false;

    // Shared state that may need to be synchronized with rvfi
    bool ncio_mem_transition_ = false;
    std::vector<mem_t> ncio_fetches_;
    std::vector<mem_t> active_ncio_fetches_;

    std::unordered_map<uint64_t, mem_t> ifetch_reqs_;
    std::unordered_map<uint64_t, mem_t> amo_writes_;
    std::unordered_map<uint64_t, mem_t> sc_result_;
    std::unordered_map<uint64_t, mem_t> sc_bypass_;

    bool mem_error_ = false;
    uint64_t ecause_ = 0;
    std::unordered_map<uint64_t, bool> vec_cmode_mem_errors_;
    bool patch_mode_ = false;
    uint64_t patch_mode_first_tag_ = 0;
    std::unordered_map<uint64_t, uint64_t> patch_mode_tags_;
    bool vec_cmode_ = false;
    std::unordered_map<uint64_t, uint64_t> vec_cmode_tags_;
    uint64_t vec_cmode_first_tag_ = 0;
    uint64_t vec_cmode_pc_addr_ = 0;

};

// Template implementation for process_split_memory_accesses
template<typename Callback>
void mcmi::process_split_memory_accesses(uint64_t base_addr, uint64_t mask,
                                         const std::bitset<256>& data_vec,
                                         Callback&& callback) {
  std::bitset<32> mask_bitset = mask;
  std::vector<uint64_t> addresses;
  std::vector<uint8_t> datas;

  // Extract addresses and data bytes from mask
  for (int i = 0; i < 32; i++) {
      if (mask_bitset[i]) {
          addresses.push_back(base_addr + i);
          uint8_t byte = 0;
          for (int bit = i*8; bit < 8*(i+1); ++bit) {
              if (data_vec[bit]) {
                  byte |= (1 << (bit - (i*8)));
              }
          }
          datas.push_back(byte);
      }
  }

  if (addresses.empty()) {
      return;
  }

  // Group consecutive addresses and call callback for each range
  uint64_t start_addr = addresses[0];
  size_t size = 1;
  std::string dataAccumulated = fmt::format("{:02x}", datas[0]);

  for (size_t i = 1; i < addresses.size(); ++i) {
      if (addresses[i] == addresses[i - 1] + 1) {
          ++size;
          dataAccumulated = fmt::format("{:02x}", datas[i]) + dataAccumulated;
      } else {
  // Process the current range
          std::bitset<256> value = stringToBitset(dataAccumulated);
          callback(start_addr, size, value, dataAccumulated);

          // Start new range
          start_addr = addresses[i];
          size = 1;
          dataAccumulated = fmt::format("{:02x}", datas[i]);
      }
  }

  // Process the final range
  std::bitset<256> value = stringToBitset(dataAccumulated);
  callback(start_addr, size, value, dataAccumulated);
}

// High-level utility function that handles both consecutive and non-consecutive memory accesses
template<typename SingleCallback, typename SplitCallback>
void mcmi::process_memory_access(uint64_t base_addr, uint64_t mask,
                                 const std::bitset<256>& data_vec,
                                 SingleCallback&& single_callback,
                                 SplitCallback&& split_callback) {
  uint64_t numones = std::popcount(mask);

  // Find the number of consecutive ones starting from the first set bit
  uint64_t leadingZeros = std::countr_zero(mask);
  uint64_t mask_shifted = mask >> leadingZeros;
  uint64_t consecutiveOnes = std::countr_zero(~mask_shifted);

  if (numones == consecutiveOnes) {
    // Single consecutive access
    single_callback();
  } else {
    // Non-consecutive, split into ranges
    process_split_memory_accesses(base_addr, mask, data_vec, std::forward<SplitCallback>(split_callback));
  }
}
