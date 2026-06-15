// Stubs for the gflags variables and one helper function that sysmod /
// trickbox / htif reference unconditionally but that are normally DEFINEd
// inside cosim source files (rvfi.cpp, bridge.cpp, mcmi.cpp, eot.cpp). With
// the open-source `cosim_off` configuration we link this small object instead
// so sysmod/trickbox/htif resolve without pulling whisper / CoreArchChecker.
//
// TODO(open-source): once cosim is restored on a public toolchain, drop this
// file and let the real DEFINE_*s in cosim/.../*.cpp do the work.

#include <cstdint>
#include "cvm/plusargs.hpp"

DEFINE_bool(cosim,                false, "cosim_off stub: cosim checking disabled");
DEFINE_bool(rvfi,                 false, "cosim_off stub: rvfi disabled");
DEFINE_bool(mcm,                  false, "cosim_off stub: mcm disabled");
DEFINE_bool(cache_model_en,       false, "cosim_off stub: cache model disabled");
DEFINE_bool(metrics,              false, "cosim_off stub: metrics disabled");
DEFINE_bool(preload,              false, "cosim_off stub: whisper preload disabled");
DEFINE_bool(standalone,           false, "cosim_off stub: whisper standalone disabled");
DEFINE_bool(cov,                  false, "cosim_off stub: arch coverage disabled");
DEFINE_bool(eot_mem_check,        false, "cosim_off stub: end-of-test mem check disabled");
DEFINE_bool(whisper_client_check, false, "cosim_off stub: whisper client check disabled");
DEFINE_string(eot,                "tohost",  "cosim_off stub: eot mechanism");
DEFINE_string(archsample_lib_path, "",        "cosim_off stub: archsample lib path");
DEFINE_uint64(max_instr,          100000, "cosim_off stub: max instruction limit");
DEFINE_uint64(tohost,             0x0,    "cosim_off stub: tohost address override");

// Real impl lives in cosim/utils/eot/eot.cpp; with cosim off, callers (sysmod
// trickbox) shouldn't actually hit the eot codepath, but the symbol still has
// to resolve at link time.
extern "C" std::uint64_t eot_get_addr() {
    return 0;
}
