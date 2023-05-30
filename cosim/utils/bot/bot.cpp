#include "bot.h"
#include <iostream>
#include "sysmod/htif/htif.h"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"

DEFINE_bool(standalone, true, "Enable  whisper standalone run at beginning of sim");
DEFINE_bool(preload, false, "Enable preload log generation");
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(bootrom_path);
DECLARE_string(whisper_path);
DECLARE_string(whisper_json_path);
DECLARE_uint32(max_instr);
DECLARE_bool(cov);
DECLARE_string(archsample_lib_path);
DEFINE_bool(iss_cov, false, "Enable iss - Arch coverage");
DECLARE_bool(terminate_call_finish);

bot::bot() 
  : archcov(ArchSample(0)) {

  if (FLAGS_standalone)
    run_iss_standalone();
}

void bot::run_iss_standalone() {
  std::string harts = " --harts 1";
  std::string config = " --configfile " + FLAGS_whisper_json_path;
  std::string trace = " --traceload --traceptw";
  std::string out_log = " --logfile iss_standalone.log";
  std::string test = (FLAGS_load != "") ? FLAGS_load : ("--hex " + FLAGS_hex);
  std::string max_inst = " --maxinst " + std::to_string(FLAGS_max_instr);
  std::string init_state = FLAGS_preload ? " --initstate preload.csv" : "";

  std::string cmd = FLAGS_whisper_path + " " + test + " " + FLAGS_bootrom_path +
  harts + config + trace + out_log + max_inst + init_state;

  if (FLAGS_iss_cov){
    // standalone whisper coverage command
    std::string cov_cmd;  
    cov_cmd = " --tracerlib " + FLAGS_archsample_lib_path + ":tracer_ext &";
    cmd += cov_cmd; 
    cvm::log(cvm::NONE, "Standalone (coverage) whisper command: {}\n", cmd);
    system(cmd.c_str());
    archcov.iss_sample();
    std::cout << "------------------Test Passed---------------------------\n";
    auto location = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal<htif::terminate_t>(location, htif::terminate_t{FLAGS_terminate_call_finish});
    //exit(0);
  } else {
    // standalone whisper 
    cvm::log(cvm::NONE, "Standalone whisper command: {}\n", cmd);
    std::string status = cosim_util::exec(cmd.c_str());
    if (status.find("Error") != std::string::npos) {
      cvm::log(cvm::NONE, "{}", status);
    }
  }
}
