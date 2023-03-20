#include "bot.h"

DEFINE_bool(standalone, true, "Enable  whisper standalone run at beginning of sim");
DEFINE_bool(preload, false, "Enable preload log generation");
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(bootrom_path);
DECLARE_string(whisper_path);
DECLARE_string(whisper_json_path);
DECLARE_int32(max_instr);
DECLARE_bool(cov);

bot::bot() {

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

  // standalone whisper 
  std::string cmd = FLAGS_whisper_path + " " + test + " " + FLAGS_bootrom_path +
    harts + config + trace + out_log + max_inst + init_state;

  cvm::log(cvm::NONE, "Standalone whisper command: {}\n", cmd);

  std::string status = cosim_util::exec(cmd.c_str());
  if (status.find("Error") != std::string::npos) {
    cvm::log(cvm::NONE, "{}", status);
  }
}
