#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "clint.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

clint::clint(const std::string& tag, uint64_t addr, unsigned hartCount,
             cvm::topology::loc_t loc)
  : device(tag, addr, 0xc000 /* size */, loc), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(hartCount, -1), timerIntPrev_(hartCount, 0), timer_(0)
{
  auto clint_loc = cvm::topology::get("CLINT", 0);
  tickDivisor_ = cvm::topology::attr(clint_loc, "clock_divisor").second;

  std::ifstream ifs;
  if (load_snapshot(ifs)) {
    std::string line;
    while (std::getline(ifs, line)) {
      // expect clint format
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


clint::~clint()
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
clint::read(uint64_t addr, size_t length, data_t& data)
{
  if (not has_addr(addr))
    return;

  uint64_t offset = addr - device::addr();
  if (offset < 0x4000) {
    // Sofware interrupt: 1 word per hart.
    if ((offset % 4) != 0)
      return;  // Address must be a multiple of 4.
    unsigned hartIx = offset / 4;
    uint32_t word = (hartIx < hartCount_) ? soft_.at(hartIx) : 0;
    serializeInt(word, length, data);
    return;
  }

  if (offset == 0xbff8) {
    serializeInt(timer_, length, data);
    return;
  }

  // Time compare. 1 double word per hart.
  if ((offset % 8) != 0)
    return;
  offset -= 0x4000;
  unsigned hartIx = offset / 8;
  uint64_t dword = hartIx < hartCount_ ? timeCompare_.at(hartIx) : 0;
  serializeInt(dword, length, data);
}


void
clint::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb)
{

  if (not has_addr(addr))
    return;

  uint64_t offset = addr - device::addr();
  if (offset < 0x4000) {
    // Sofware interrupt: 1 word per hart.
    unsigned hartIx = offset / 4;
    if ((offset % 4) != 0 or length < 4 or hartIx >= hartCount_)
      return;
    unsigned word = 0;
    deserializeInt(data, word);
    soft_.at(hartIx) = word & 1;
    softwareInterrupt(hartIx, word & 1);
  }

  std::cout<<"\nlength before loop: 0x"<<std::hex<<length;
  if (length == 8) {
    offset -= 0x4000;
  std::cout<<"\nlength before loop: 0x"<<std::hex<<length;
  //std::cout<<"\ndata before loop: 0x"<<std::hex<<data;
  std::cout<<"\ntime before loop: 0x"<<std::hex<<timer_;
  std::cout<<"\noffset before loop: 0x"<<std::hex<<offset;
  //std::cout<<"\nhartIx: 0x"<<std::hex<<hartIx;
  std::cout<<"\nhartCount: 0x"<<std::hex<<hartCount_; 
    if (offset == 0x7ff8)
      deserializeInt(data, timer_);
    else {
      unsigned hartIx = offset / 8;
      if (hartIx >= hartCount_)
        return;

      // Time compare. 1 double word per hart.
      uint64_t dword = 0;
      deserializeInt(data, dword);
      timeCompare_.at(hartIx) = dword;
  std::cout<<"\nlength after loop: 0x"<<std::hex<<length;
  std::cout<<"\ndata after loop: 0x"<<std::hex<<dword;
  std::cout<<"\ntime after loop: 0x"<<std::hex<<timer_;
  std::cout<<"\noffset after loop: 0x"<<std::hex<<offset;
  std::cout<<"\nhartIx: 0x"<<std::hex<<hartIx;
  std::cout<<"\nhartCount: 0x"<<std::hex<<hartCount_;
    }

    processTimerInterrupts();
  }
}

void clint::tick(uint64_t advance)
{
  if ((advance % tickDivisor_) != 0) {
    cvm::log(cvm::NONE, "ERROR: Clock advancing by {}, not a multiple of configured divisor {}", advance, tickDivisor_);
  }
  timer_ += advance / tickDivisor_;
  processTimerInterrupts();
}
