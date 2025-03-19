#include <cstdlib>
#include <string_view>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "memmap.h"
#include "rv_tester_transactions.hpp"
#include "rv_tester/rv_tester_structs.h"
#include "sysmod/sysmod_plusargs.h"
#include <fmt/format.h>
#include "cosim/utils/eot/eot_plusargs.h"

static bool validate_ge0(const char* flagname, const int value) {
    if (value < 0) {
        cvm::log(cvm::NONE, "Invalid value for +{}={}, must be >= 0\n", flagname, value);
        return false;
    }
    return true;
}

// NORMAL
// static void convert_csv_to_hex(const std::string& csv_path, const std::string& hex_path) {
//     std::ifstream csv(csv_path);
//     if (!csv.is_open()) {
//         std::cerr << "Error: could not open CSV file: " << csv_path << std::endl;
//         return;
//     }
//     std::vector<std::string> data_lines;
//     std::vector<std::string> tag_lines;
//     std::string line;
//     while (std::getline(csv, line)) {
//         if (line.empty())
//             continue;
//         std::istringstream iss(line);
//         std::string type, va_str, pa_str, data_str, cacheable_str;
//         std::getline(iss, type, ',');
//         std::getline(iss, va_str, ',');
//         std::getline(iss, pa_str, ',');
//         std::getline(iss, data_str, ',');
//         std::getline(iss, cacheable_str, ',');
//         data_lines.push_back(data_str);
//         // For a physically tagged cache, use the physical address (pa) to compute the tag.
//         //    index_bits = 3 (for 8 lines)
//         //    block_offset_bits = 8 (for 4 blocks of 8 bytes = 32 bytes per cache line)
//         const int index_bits = 3;
//         const int block_offset_bits = 8;
//         std::uint64_t pa;
//         std::istringstream(pa_str) >> std::hex >> pa;
//         std::uint64_t tag = pa >> (index_bits + block_offset_bits);
//         std::stringstream tag_ss;
//         tag_ss << std::setw(8) << std::setfill('0') << std::hex << tag;
//         tag_lines.push_back(tag_ss.str());
//     }
//     csv.close();
    
//     std::ofstream hex(hex_path);
//     if (!hex.is_open()) {
//         std::cerr << "Error: could not open HEX file for writing: " << hex_path << std::endl;
//         return;
//     }
//     // Write data section: each data field on its own line.
//     for (const auto &d : data_lines) {
//         hex << d << "\n";
//     }
//     // Write tag section: each computed tag on its own line.
//     for (const auto &t : tag_lines) {
//         hex << t << "\n";
//     }
//     hex.close();
//     std::cout << "Converted CSV " << csv_path << " to HEX file " << hex_path << std::endl;
// }

// ARRAY OF FILES
struct PreloadFiles {
    std::vector<std::string> dataFiles;
    std::vector<std::string> tagFiles;
};

PreloadFiles convert_csv_to_preload_files_per_way(const std::string& csv_path, unsigned numWays) {
    std::ifstream csv(csv_path);
    if (!csv.is_open()) {
        std::cerr << "Error: could not open CSV file: " << csv_path << std::endl;
        return PreloadFiles{};
    }
    
    std::vector<std::string> dataLines;
    std::vector<std::string> tagLines;
    std::string line;
    while (std::getline(csv, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string type, va, pa, data, cacheable;
        std::getline(iss, type, ',');
        std::getline(iss, va, ',');
        std::getline(iss, pa, ',');
        std::getline(iss, data, ',');
        std::getline(iss, cacheable, ',');
        dataLines.push_back(data);
        const int index_bits = 3;
        const int block_offset_bits = 8;
        std::uint64_t pa_val = 0;
        std::istringstream iss_pa(pa);
        iss_pa >> std::hex >> pa_val;
        std::uint64_t tag = pa_val >> (index_bits + block_offset_bits);
        std::stringstream tag_ss;
        tag_ss << std::setw(8) << std::setfill('0') << std::hex << tag;
        tagLines.push_back(tag_ss.str());
    }
    csv.close();
    
    // Create per-way vectors and distribute rows round-robin.
    std::vector<std::vector<std::string>> perWayData(numWays);
    std::vector<std::vector<std::string>> perWayTag(numWays);
    for (size_t i = 0; i < dataLines.size(); i++) {
        unsigned way = i % numWays;
        perWayData[way].push_back(dataLines[i]);
        perWayTag[way].push_back(tagLines[i]);
    }
    
    // Write out each way's data and tag files.
    PreloadFiles pf;
    for (unsigned w = 0; w < numWays; w++) {
        std::string dataFilename = "preload_data_way" + std::to_string(w) + ".hex";
        std::ofstream dataFile(dataFilename);
        if (!dataFile.is_open()) {
            std::cerr << "Error: could not open output file " << dataFilename << std::endl;
            continue;
        }
        for (const auto& row : perWayData[w]) {
            dataFile << row << "\n";
        }
        dataFile.close();
        pf.dataFiles.push_back(dataFilename);
        std::cout << "Created preload data file for way " << w << ": " << dataFilename << std::endl;
        
        std::string tagFilename = "preload_tag_way" + std::to_string(w) + ".hex";
        std::ofstream tagFile(tagFilename);
        if (!tagFile.is_open()) {
            std::cerr << "Error: could not open output file " << tagFilename << std::endl;
            continue;
        }
        for (const auto& row : perWayTag[w]) {
            tagFile << row << "\n";
        }
        tagFile.close();
        pf.tagFiles.push_back(tagFilename);
        std::cout << "Created preload tag file for way " << w << ": " << tagFilename << std::endl;
    }
    return pf;
}

DEFINE_int32(perf_period, 0, "cycles to wait to report clock performance");
DEFINE_int32(quiesce_timeout, 500, "cycles to wait after eot condition before calling $finish");
DEFINE_int32(flush_timeout, 25000, "cycles to wait after flush is initiated before calling $finish");
DEFINE_bool(terminate_call_finish, true, "Call $finish on sim termination");
DEFINE_bool(bypass_mem, true, "Bypass xbar+cache switch");
DEFINE_bool(bypass_cache, false, "Bypass cache switch");
DEFINE_int32(num_reruns, 0, "Rerun the same test this many times, to test test chaining for emulation. The test is run for a total of N+1 times.");
DEFINE_bool(trace_en, false, "Set this while running trace test");
DEFINE_bool(overlay_mmr_en, false, "Set this while running overlay test");
DEFINE_bool(jtag_en, false, "Set this while running jtag test");
DEFINE_bool(smc_sweep_test ,false, "Set this while running small core sram sweep test");
DEFINE_int32(dmi_poll_timeout, 50000, "Debug poll timeout after to host end call");
DEFINE_int32(ndmreset_ack_delay, 0, "Delay after which ndmreset ack is asserted");
DEFINE_int32(trace_timeout, 50000, "trace test end timeout after to host end call");
DEFINE_int32(freq_switch_ncycles, 20000, "Switch clk frequencies after freq_switch_ncycles");
DEFINE_int32(clk_profile, 0, "Clk profile to drive various clocks");
DEFINE_bool(dyn_clk_switch, false, "Enable dynamic clk switching");
DEFINE_validator(num_reruns, &validate_ge0);
DEFINE_string(gen_clocks_verbosity, "HIGH", "verbosity at which to generate clocks with cvm::logger prints");
DEFINE_int32(assertion_test_cycle, 0, "If non-zero, assert false on this cycle. Used for testing assertion infrastructure.");
DEFINE_int32(rand_dmi_driver_dly, 0, "Random delay cycles, to be used while driving DMI transactions");
DEFINE_int32(dm_single_step_count, 0, "No of times core to single step, to be used while driving DMI transactions");
DEFINE_int32(sdtrig_multitrigger, 0, "No of trigger condigurations for sdtrig multitrigger test");
DEFINE_int32(trigger_config, 0, "No of store addr configurations for sdtrig test");
DEFINE_bool(priority_singlestep, false, "Prioritize single step over sdtrig in cross feature verification test");
DEFINE_bool(rv_tester_terminate, true, "Set to false for offline DPI mode");
DEFINE_string(preload_file, "", "Preload file for LLC");

extern "C" void rv_tester_terminate();
extern "C" void rv_tester_set_address_map_and_preload_file(std::uint32_t i, std::uint64_t start_addr, std::uint64_t end_addr, std::uint32_t device, const char* preload_file);
extern "C" void rv_tester_preload_file(const char* preload_file);
extern "C" void set_preload_data_file(std::uint32_t way, const char* file);
extern "C" void set_preload_tag_file(std::uint32_t way, const char* file);

static bool check_called;
class logger_instrument {

    public:
        logger_instrument(cvm::topology::loc_t loc, unsigned) : loc(loc) {};

        void configure() {
            clock = 0;

            cvm::set_logger_prefix([]() -> std::string_view {
                prefix = (clock)? "[" + std::to_string(clock) + "] " : "";
                return prefix;
            });

            cvm::registry::messenger.connect<rv_tester_transactions::logger::cycle<>>(loc, [] (const auto& c) { clock = c.clock; });
        }

        void check() {
            // we want this to be low prio and async so it goes behind existing rvfi transactions in the queue
            // because of QoS this could have been seen before all rvfi transactions up to this instruction were processed
            cvm::registry::messenger.signal_async<rv_tester::terminate_called>(loc, rv_tester::terminate_called{}, cvm::messenger::lowest_priority);
            if (FLAGS_rv_tester_terminate) {
                cvm::registry::messenger.signal_async<rv_tester::terminate_called>(loc, rv_tester::terminate_called{}, cvm::messenger::lowest_priority);
                cvm::registry::callbacks.push(
                    scope,
                    []() {
                        return rv_tester_terminate();
                    });
            }
        }

        static void set_scope(svScope s) { scope = s; };

    private:

        static svScope scope;
        static std::string prefix;
        static uint64_t clock;
        cvm::topology::loc_t loc;
};

extern "C" {

    int rv_tester_perf_calc(int init, int reset_done, int terminate, std::uint64_t clocks) {
        //cvm::log(cvm::NONE, "rv_tester_perf_calc(init={} terminate={}  clocks={})\n", init,terminate,clocks);
        static std::chrono::time_point<std::chrono::high_resolution_clock> zero_time;
        static std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        static std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
        static std::chrono::time_point<std::chrono::high_resolution_clock> rstdone_time;
        int pcps,tcps,period;
        static std::uint64_t first_clk, rdone_clk;
        static std::uint64_t last_clk;

        end_time = std::chrono::high_resolution_clock::now();

        if (init==1) {
            first_clk = clocks;
            last_clk  = clocks;
            zero_time = end_time; 
            start_time = end_time;
            return(1);
        }


        if (reset_done==1) {
           auto rduration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - zero_time).count();
           pcps = 0;
           period = clocks - first_clk;
           rstdone_time = end_time;
           rdone_clk  = clocks;
           if (rduration > 0) {
              pcps = (int)(period/rduration);
           }
           cvm::log(cvm::NONE, "time={}  reset_performance_khz\": {}\n", clocks,pcps);
           start_time = end_time; 
           last_clk = clocks;
           return(1);
        }

        if (terminate==1) {
           auto tduration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - rstdone_time).count();
           period = clocks - rdone_clk;
           if (tduration > 0) {
              tcps = (int)(clocks/tduration);
           }
           cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"exec_performance_khz\": {}}}\n", tcps);

           tduration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - zero_time).count();
           tcps = 0;
           period = clocks - first_clk;
           if (tduration > 0) {
              tcps = (int)(clocks/tduration);
           }
           cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"test_performance_khz\": {}}}\n", tcps);
           last_clk = clocks;
           start_time = end_time; 
           return(1);
        }

        auto pduration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        pcps = 0;
        if (pduration > 0) { 
           period = clocks - last_clk;
           pcps = (int)(period/pduration);
        }
        cvm::log(cvm::NONE, "time:{}  period_performance_khz\": {}\n", clocks,pcps);
        start_time = end_time; 
        last_clk = clocks;
        return(1);

    }

    int rv_tester_parse_flags() {
        cvm::log(cvm::NONE, "[plusargs] Parsing...\n");
        cvm::plusargs::parse();
        return 0;
    }

    void rv_tester_set_seed() {
        cvm::log(cvm::NONE, "[random] +seed={}\n", FLAGS_seed);
        cvm::rand::seed(FLAGS_seed);
    }

    void rv_tester_parse_memmap(std::uint32_t no_addr_rules) {

        std::map<std::string, memmap_entry_t> m;
        if (!cvm::registry::messenger.call<memmap::getRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.MEMMAP", 0), m))
            return;
        if (m.size() > no_addr_rules) {
            cvm::log(cvm::ERROR, "Test specifying more address rules ({}) than in sv ({})", m.size(), no_addr_rules);
            return;
        }

        std::string preloadStr = FLAGS_preload_file;
        // Get the preload file string from plusargs.
        // std::string preloadStr = FLAGS_preload_file;
        // If the preload file ends with ".csv", convert it into per-way HEX files.
        if (!preloadStr.empty() && preloadStr.substr(preloadStr.size()-4) == ".csv") {
            unsigned numWays = 4;
            PreloadFiles pf = convert_csv_to_preload_files_per_way(preloadStr, numWays);
            if (pf.dataFiles.empty() || pf.tagFiles.empty()) {
                cvm::log(cvm::ERROR, "CSV conversion failed; no preload files generated.");
                return;
            }
            for (unsigned w = 0; w < numWays; w++) {
                set_preload_data_file(w, pf.dataFiles[w].c_str());
                set_preload_tag_file(w, pf.tagFiles[w].c_str());
            }
            preloadStr = pf.dataFiles[0];
        }

        std::uint32_t i = 0;
        for (const auto& it : m) {
            const auto& e = it.second;
            rv_tester_set_address_map_and_preload_file(i, e.base, e.end, e.type != "memory", preloadStr.c_str());
            rv_tester_preload_file(preloadStr.c_str());
            i++;
        }
        for(; i < no_addr_rules; i++) {
            rv_tester_set_address_map_and_preload_file(i, 1, 1, 1, preloadStr.c_str());
            rv_tester_preload_file(preloadStr.c_str());
        }
    }

    void rv_tester_build_registry() {
        check_called = false;
        cvm::registry::build();
        cvm::registry::configure();
    }
   
    void rv_tester_no_dm_build_registry() {
        auto dm_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.DM_MODEL", 0);
        check_called = false;
        cvm::registry::build_all_except(dm_loc);
        cvm::registry::configure();
    } 
    void rv_tester_dm_build_registry() {
        auto dm_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.DM_MODEL", 0);
        cvm::log(cvm::NONE, "[registry] build dm components ...\n");
        cvm::registry::build(dm_loc);//pass dm location
        //cvm::registry::configure();//pass dm location
    }

    uint8_t rv_tester_shutdown_registry() {
        auto dm_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.DM_MODEL", 0);
        if (!check_called) {
            cvm::log(cvm::NONE, "[registry] check...\n");
            cvm::registry::check();
            check_called = true;
        }

        cvm::log(cvm::NONE, "[registry] shutdown...\n");
        //return cvm::registry::shutdown();
        return cvm::registry::shutdown_all_except(dm_loc);
    }
    
    uint8_t rv_tester_dm_shutdown_registry() {
        auto dm_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.DM_MODEL", 0);
        cvm::log(cvm::NONE, "[registry] dm shutdown check...\n");
 

        cvm::log(cvm::NONE, "[registry] dm shutdown...\n");
        return cvm::registry::shutdown(dm_loc);//dm location
    }

    uint8_t rv_tester_flush_callbacks() {
        cvm::registry::callbacks.flush();
        // force verilator to serialize
        return true;
    }

    void rv_tester_cvm_error() {
        if (!check_called) {
            cvm::registry::check();
        }
    }

    void rv_tester_cvm_error_handler() {
        logger_instrument::set_scope(svGetScope());
        cvm::set_logger_handler(cvm::ERROR, rv_tester_cvm_error);
    }

    void rv_tester_streaming_dpi_init() {
        char *env_var = std::getenv("ZEBU_OFFLINE_DPI");
        if ((env_var != nullptr && std::string(env_var) == "1")) {
            cvm::log(cvm::NONE, "Initialize Offline DPI\n");
            rv_tester_parse_flags();

            // override flags in offline mode
            FLAGS_sysmod_terminate = false;
            FLAGS_rv_tester_terminate = false;
            FLAGS_signal_async = false;
            FLAGS_hw_eot_enable = false;

            rv_tester_cvm_error_handler();
            rv_tester_build_registry();
            std::atexit([] () {
                while(!rv_tester_shutdown_registry());
                rv_tester_dm_shutdown_registry();
            });
        }
    }
}

svScope logger_instrument::scope;
std::string logger_instrument::prefix;
uint64_t logger_instrument::clock;

REGISTRY_register(logger_instrument, TOP.PLATFORM, 0);
