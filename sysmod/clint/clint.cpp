#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "clint.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

DECLARE_string(device_load_dir);
DECLARE_string(device_save_dir);

clint::clint(const std::string& tag, uint64_t addr, unsigned hartCount,
             cvm::topology::loc_t loc)
  : device(tag, addr, 0xc000 /* size */, loc), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(hartCount), timerIntPrev_(hartCount, 0), timer_(0)
{
  if (not FLAGS_device_load_dir.empty()) {
    std::ifstream ifs(FLAGS_device_load_dir + "/" + tag);
    if (not ifs) {
      cvm::log(cvm::LOW, "Could not find " + tag + " under snapshot save directory, will skip\n");
    }
    else {
      std::string line;
      while (std::getline(ifs, line)) {
        // expect clint format
        std::string type, val;

        std::istringstream iss(line);
        assert(iss >> type);

        if (type == "timer")
          timer_ = strtoull(val.c_str(), nullptr, 0);
        else if (type == "cmp") {
          assert(iss >> val);
          uint64_t num = strtoull(val.c_str(), nullptr, 0);
          assert(iss >> val);
          // TODO: error check number < hartCount
          timeCompare_.at(num) = strtoull(val.c_str(), nullptr, 0);
        }
        else {
          cvm::log(cvm::NONE, "Error: unrecognized line " + type + " in " + tag + " snapshot dir\n");
        }
      }

      ifs.close();
    }
  }
}


clint::~clint()
{
  if (not FLAGS_device_save_dir.empty()) {
    std::ofstream ofs(FLAGS_device_save_dir + "/" + tag());
    if (not ofs) {
      cvm::log(cvm::NONE, "Error: Could not open " + FLAGS_device_save_dir + " for saving snapshot\n");
    }
    else {
      // no point in saving interrupt status
      ofs << "timer " << std::dec << timer_ << '\n';
      for (unsigned i = 0; i < timeCompare_.size(); i++) {
        ofs << "cmp " << std::dec << i << " " << timeCompare_.at(i) << '\n';
      }
    }
  }
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

  if (length == 8) {
    offset -= 0x4000;

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
    }

    processTimerInterrupts();
  }
}
