#include "src/sysmod/device.h"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

DEFINE_string(device_load_dir, "", "Path to load device snapshots");
DEFINE_string(device_save_dir, "", "Path to save device snapshots");

bool
device::load_snapshot(std::ifstream& ifs) {
  if (FLAGS_device_load_dir.empty())
    return false;

  ifs.open(FLAGS_device_load_dir + "/" + tag_);
  if (not ifs) {
    cvm::log(cvm::HIGH, "Could not find " + tag_ + " under snapshot load directory, will skip\n");
    return false;
  }
  else
    return true;
}

bool
device::save_snapshot(const std::stringstream& ss) {
  if (FLAGS_device_save_dir.empty())
    return false;

  std::ofstream ofs(FLAGS_device_save_dir + "/" + tag_);
  if (not ofs) {
    cvm::log(cvm::HIGH, "Error: Could not open " + FLAGS_device_save_dir + " for saving snapshot\n");
    return false;
  }
  else {
    ofs << ss.rdbuf();
    ofs.close();
    return true;
  }
}
