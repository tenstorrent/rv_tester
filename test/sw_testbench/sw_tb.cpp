// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// Verilator top-level C++ driver for generic top built with --timing
//
// Waveform dumping (dbg builds compiled with a VERILATOR_<fmt> local_define) is
// opt-in via plusargs:
//   +fsdb_dump_on=<t>   enable dumping starting at time <t> (in timeunits)
//   +fsdb_dump_off=<t>  stop dumping at time <t>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "verilated.h"

#include "Vtop.h"

#include "cvm/logger.hpp"

// Waveform format selection. The build may set VERILATOR_WAVES plus one of the
// format macros; derive VERILATOR_WAVES purely from the format actually present.
#undef VERILATOR_WAVES
#if defined(VERILATOR_FSDB)
#include "verilated_fsdb_c.h"
using VltTraceFile = VerilatedFsdbC;
static constexpr char TRACE_FILE_NAME[] = "dump.fsdb";
#define VERILATOR_WAVES
#elif defined(VERILATOR_FST)
#include "verilated_fst_c.h"
using VltTraceFile = VerilatedFstC;
static constexpr char TRACE_FILE_NAME[] = "dump.fst";
#define VERILATOR_WAVES
#elif defined(VERILATOR_VCD)
#include "verilated_vcd_c.h"
using VltTraceFile = VerilatedVcdC;
static constexpr char TRACE_FILE_NAME[] = "dump.vcd";
#define VERILATOR_WAVES
#elif defined(VERILATOR_SAIF)
#include "verilated_saif_c.h"
using VltTraceFile = VerilatedSaifC;
static constexpr char TRACE_FILE_NAME[] = "dump.saif";
#define VERILATOR_WAVES
#endif

static uint32_t tb_clk_freq_mhz = 0;

extern "C" void sw_tb_set_tb_clk_freq_mhz(const uint32_t freq_mhz) {
  tb_clk_freq_mhz = freq_mhz;
}

int main(int argc, char** argv) {
  VerilatedContext context;
  context.debug(0);
  context.threads(1);
  // Must run before constructing the model so $value$plusargs sees args.
  context.commandArgs(argc, argv);

  // Passing the empty string as instance name suppresses tracing
  // of the top level wrapper, which is what we want to match other
  // simulators.
  Vtop top{&context, ""};

  // Simulation timescale
  const int timeunit = context.timeunit();           // between 0 (1s) .. -15 (1fs)
  const int timeprecision = context.timeprecision(); // between 0 (1s) .. -15 (1fs) <= timeunit
  // timeunit -> timeprecision factor
  const uint64_t timeUnit2Prec = static_cast<uint64_t>(std::pow(10.0, -timeprecision - -timeunit));

  //------------------------------------------------------------
  // Dump configuration

  // Figure out when to turn on dumping
  bool dumping = true;
  bool dump_on_in_cycles = false;
  uint64_t dump_on = 0;
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_on=")) {
    dump_on = std::stoull(valp);
    dump_on_in_cycles = true;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_on=")) {
    dump_on = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_on=")) {
    // Accept vcd_cycle_on for backward compatibility
    dump_on = std::stoull(valp);
    dump_on_in_cycles = true;
  } else {
    dumping = false;
  }

  // Figure out when to turn off dumping
  bool dump_off_in_cycles = false;
  uint64_t dump_off = std::numeric_limits<uint64_t>::max();
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_off=")) {
    dump_off = std::stoull(valp);
    dump_off_in_cycles = true;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_off=")) {
    dump_off = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_off=")) {
    // Accept vcd_cycle_off for backward compatibility
    dump_off = std::stoull(valp);
    dump_off_in_cycles = true;
  }

#ifdef VERILATOR_WAVES
  std::unique_ptr<VltTraceFile> tfp; // The dump file
  if (dumping) {
    context.traceEverOn(true);
    tfp.reset(new VltTraceFile{});
  }
#else
  if (dumping) {
    cvm::log(cvm::LOW, "Warning: wave dump requested but model compiled without dumping capability\n");
  }
#endif

  // A dump at time t is emitted only while dump_on <= t <= dump_off.
  auto dump_at = [&](uint64_t t) {
#ifdef VERILATOR_WAVES
    if (dumping) {
      if (t >= dump_on && t <= dump_off) {
        tfp->dump(t);
      }
      if (tfp->isOpen() && t >= dump_off) {
        tfp->flush();
        tfp->close();
      }
    }
#else
    (void)t;
#endif
  };

  // First eval before registering the trace: with a partitioned model the
  // partition instances are only created/added to the context after the first
  // eval (Verilator behavior), so trace callbacks must be registered after it.
  top.eval();

  // Numerator to compute period for a MHz frequency at the current timeprecision
  const uint64_t mhz2periodNumer = static_cast<uint64_t>(std::pow(10.0, -timeprecision - 6));
  const uint64_t tb_clk_period = tb_clk_freq_mhz ? mhz2periodNumer / tb_clk_freq_mhz : 0;
  if (tb_clk_freq_mhz && !tb_clk_period) {
    cvm::log(cvm::ERROR, "Error: Insufficient timeprecision 1e{} for {}MHz TB clock\n", timeprecision, tb_clk_freq_mhz);
    exit(1);
  }
  const auto cycles_to_time = [tb_clk_period](const uint64_t cycles) {
    if (!tb_clk_period) {
      cvm::log(cvm::ERROR,
               "Error: cycle-based dump plusargs need the TB clock frequency registered by top.sv; "
               "use the time-based (+*_dump_on/off) form instead\n");
      exit(1);
    }
    return cycles * tb_clk_period;
  };
  if (dump_on_in_cycles) {
    dump_on = cycles_to_time(dump_on);
  }
  if (dump_off_in_cycles) {
    dump_off = cycles_to_time(dump_off);
  }
  // Make sure we are not doing something stupid, the time queue assumes this
  if (dump_on >= dump_off) {
    cvm::log(cvm::ERROR, "Error: dump on time ({}) must be before dump off time ({})\n", dump_on, dump_off);
    exit(1);
  }
#ifdef VERILATOR_WAVES
  if (dumping) {
    context.trace(tfp.get(), 99, 0);
    tfp->open(TRACE_FILE_NAME);
  }
#endif
  dump_at(context.time());

  // Timing event loop: advance to the next scheduled event until $finish.
  while (!context.gotFinish() && top.eventsPending()) {
    context.time(top.nextTimeSlot());
    top.eval();
    dump_at(context.time());
  }

  if (!context.gotFinish()) {
    VL_PRINTF("%%Warning: top: no $finish - ran out of events\n");
  }

#ifdef VERILATOR_WAVES
  if (dumping && tfp->isOpen()) {
    tfp->flush();
    tfp->close();
  }
#endif

  top.final();
  return 0;
}

extern "C" void assert_on_dpi() {
  Verilated::assertOn(true);
}

extern "C" void assert_off_dpi() {
  Verilated::assertOn(false);
}
