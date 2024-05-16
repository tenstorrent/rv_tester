#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "trace_cfg.h"
#include "transactors/axi_sw/axi.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>


DECLARE_string(load);
DECLARE_int32(seed);


trace_cfg::trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &trace_cfg::write, &trace_cfg::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void trace_cfg::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  trace_wr_t wr;

  wr = trace_wr_txn_q.front();
  trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  addr = addr & 0xFFFFFFC0;
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void trace_cfg::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   //cvm::registry::messenger.signal(loc(), trace_cfg_read_t{addr, length, id});
   //cvm::log(cvm::HIGH, "[axi_read prints] cnt_tick {} start_trace_cnt {} \n",cnt_tick,start_trace_cnt);
    cvm::log(cvm::FULL, "[TRACE CFG] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  auto* l = +[](transactor::read_t r, trace_cfg* dev) -> cvm::messenger::task<void>{
    data_t d;
    co_await dev->read(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);
}


void trace_cfg::write(const transactor::write_t& ) {
  // auto& addr = w.addr;
  // auto& length = w.length;
  // auto& data = w.data;
  // auto& strb = w.strb;

  // cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  // return;
}

// Function to parse a file and extract #define macros
std::unordered_map<std::string, uint32_t> trace_cfg::extractMacros(const std::string& filename) {
    std::unordered_map<std::string, uint32_t> macros;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return macros;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "#define") {
            std::string macroName;
            std::string macroValue;
            iss >> macroName >> macroValue;
            // std::cout<<"[value]"<<macroValue;
            macros[macroName] = std::stoi(macroValue,0,16);
        }
    }

    file.close();
    return macros;
}

std::unordered_map<std::string, uint32_t> trace_cfg::pickRandomElements(const std::unordered_map<std::string, uint32_t>& originalMap, uint32_t n) {
    std::unordered_map<std::string, uint32_t> result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::cout << "originalMap.size() " << originalMap.size() << std::endl;
    std::uniform_int_distribution<> dis(0, originalMap.size() - 1);

    std::vector<std::pair<std::string, uint32_t>> elements;

    for (const auto& pair : originalMap) {
        elements.push_back(pair);
    }

    for (uint32_t i = 0; i < n; ++i) {
        int randomIndex = dis(gen);
        result.insert(elements[randomIndex]);
    }
    std::cout << "random result.size() " << result.size() << std::endl;
    return result;
}

cvm::messenger::task<void> trace_cfg::read(const transactor::read_t& r, data_t& ) {
  auto& addr = r.addr;
  auto& length = r.length;

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  trace_cfg_read_req_t trace_cfg_rd;
  trace_cfg_rd.addr = addr;
  trace_cfg_rd.length = length;
  trace_cfg_rd.id = r.id;
  trace_cfg_rd.data = resp.data;  
  trace_read_resp_q.push(trace_cfg_rd); 
  co_return;
}

void trace_cfg::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void trace_cfg::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool trace_cfg::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
