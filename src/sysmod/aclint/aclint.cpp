#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "src/sysmod/aclint/aclint.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

aclint::aclint(const std::string& tag, uint64_t addr, unsigned hartCount,
             cvm::topology::loc_t loc)
  : device(tag, addr, 0xc000 /* size */, loc, &aclint::write, &aclint::read, this), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(hartCount, -1), timerIntPrev_(hartCount, 0), timer_(0)
{
  auto clint_loc = cvm::topology::get_from_type("CLINT", 0);
  tickDivisor_ = cvm::topology::attr(clint_loc, "CLOCK_DIVISOR").second;
  
  std::ifstream ifs;
  if (load_snapshot(ifs)) {
    std::string line;
    while (std::getline(ifs, line)) {
      // expect aclint format
      std::string type, val;

      std::istringstream iss(line);
      iss >> type;

      if (type == "timer")
        timer_ = strtoull(val.c_str(), nullptr, 0);
      else if (type == "cmp") {
        iss >> val;
        uint64_t num = strtoull(val.c_str(), nullptr, 0);
        iss >> val;
        // TODO: error check number < hartCount
        timeCompare_.at(num) = strtoull(val.c_str(), nullptr, 0);
      }
      else {
        cvm::log(cvm::NONE, "Error: unrecognized line " + type + " for " + tag + "\n");
      }
    }

    ifs.close();
  }
}


aclint::~aclint()
{
  std::stringstream ss;
  // no point in saving interrupt status
  ss << "timer " << std::dec << timer_ << '\n';
  for (unsigned i = 0; i < timeCompare_.size(); i++) {
    ss << "cmp " << std::dec << i << " " << timeCompare_.at(i) << '\n';
  }

  save_snapshot(ss);
}


void
aclint::read(const transactor::read_t& r, data_t& data)
{
  auto& addr = r.addr;
  auto& length = r.length;

  uint64_t offset = addr - device::addr();

  if (offset == 0) {
    serializeInt(timer_, length, data);
    return;
  }

  // Time compare. 1 double word per hart.
  if ((offset % 8) != 0)
    return;
  offset -= 0x8000;
  unsigned hartIx = offset / 8;
  uint64_t dword = (hartIx >= 0 && hartIx < hartCount_) ? timeCompare_.at(hartIx) : 0;
  serializeInt(dword, length, data);
  return;
}


void
aclint::write(const transactor::write_t& w)
{
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;

// MMR_BASE_ADDR, 2'b01, ClusterID, 5'b10011,16'hFFFF
// 1010 0001 0011 0000 0000 0000 0000
// 1010 0001 0011 1111 1111 1111 1111
//.equ mtime, 0xa130000 
//.equ mtimecmp, 0xa138000

  uint64_t offset = addr - device::addr();

  if (length == 8) {

    if (offset == 0)
      deserializeInt(data, timer_);
    else if (offset == 0x8000){
      offset -= 0x8000;
      unsigned hartIx = offset / 8;
      if (hartIx < 0 && hartIx >= hartCount_)
        return;

      // Time compare. 1 double word per hart.
      uint64_t dword = 0;
      deserializeInt(data, dword);
      timeCompare_.at(hartIx) = dword;
    }

    processTimerInterrupts();
  }
}

void aclint::tick(uint64_t advance)
{
  if ((advance % tickDivisor_) != 0) {
    cvm::log(cvm::ERROR, "Error: Clock advancing by {}, not a multiple of configured divisor {}\n", advance, tickDivisor_);
  }
  timer_ += (advance / tickDivisor_)*10;
  processTimerInterrupts();
}
