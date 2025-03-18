#include "cvm/plusargs.hpp"

DEFINE_bool(standalone, true, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(load_lz4, "", "lz4 compressed file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_string(load_bin, "", "Binary file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_bool(bootrom, true, "Load bootrom before test");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_bool(cplfw, false, "Load cpl firmware before test");
DEFINE_string(cplfw_path, "", "Path to cpl firmware object file");
DEFINE_bool(preload, false, "Whisper preload");
DEFINE_uint64(tohost, 0x0, "Use this tohost address if provided");
DEFINE_uint64(max_instr, 100000, "Max instruction limit to terminate the sim");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(mcm, true, "Enable mcm");

