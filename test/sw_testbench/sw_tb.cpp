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
#include "cvm/registry.hpp"
#include "rv_tester_util/clock_profiles.h"

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
  uint64_t dump_on_cycles = 0;
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_on=")) {
    dump_on_cycles = std::stoull(valp);
    dump_on_in_cycles = dump_on_cycles != 0;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_on=")) {
    dump_on = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_on=")) {
    // Accept vcd_cycle_on for backward compatibility
    dump_on_cycles = std::stoull(valp);
    dump_on_in_cycles = dump_on_cycles != 0;
  } else {
    dumping = false;
  }

  // Figure out when to turn off dumping
  bool dump_off_in_cycles = false;
  uint64_t dump_off = std::numeric_limits<uint64_t>::max();
  uint64_t dump_off_cycles = 0;
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_off=")) {
    dump_off_cycles = std::stoull(valp);
    dump_off_in_cycles = true;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_off=")) {
    dump_off = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_off=")) {
    // Accept vcd_cycle_off for backward compatibility
    dump_off_cycles = std::stoull(valp);
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
    if (dumping && !dump_on_in_cycles) {
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

  const auto clki = cvm::topology::get_from_type("CLKI", 0);
  const auto tb_clk_idx_attr = cvm::topology::attr(clki, "TB_CLK_IDX");
  const uint32_t tb_clk_idx = tb_clk_idx_attr.first ? tb_clk_idx_attr.second : 0;
  // Numerator to compute period for a MHz frequency at the current timeprecision
  const uint64_t mhz2periodNumer = static_cast<uint64_t>(std::pow(10.0, -timeprecision - 6));
  const auto check_dump_window = [&]() {
    // Make sure we are not doing something stupid, the time queue assumes this
    if (dump_on >= dump_off) {
      cvm::log(cvm::ERROR, "Error: dump on time ({}) must be before dump off time ({})\n", dump_on, dump_off);
      exit(1);
    }
  };
  bool dump_bounds_pending = dump_on_in_cycles || dump_off_in_cycles;
  const auto resolve_dump_bounds = [&]() {
    if (tb_clk_idx >= rv_tester::clocks::nclks()) {
      return;
    }
    const uint32_t tb_clk_freq_mhz = rv_tester::clocks::freq_mhz(0, tb_clk_idx);
    const uint64_t tb_clk_period = tb_clk_freq_mhz ? mhz2periodNumer / tb_clk_freq_mhz : 0;
    if (!tb_clk_period) {
      cvm::log(cvm::ERROR, "Error: Insufficient timeprecision 1e{} for {}MHz TB clock\n", timeprecision, tb_clk_freq_mhz);
      exit(1);
    }
    if (dump_on_in_cycles) {
      dump_on = dump_on_cycles * tb_clk_period;
      dump_on_in_cycles = false;
    }
    if (dump_off_in_cycles) {
      dump_off = dump_off_cycles * tb_clk_period;
      dump_off_in_cycles = false;
    }
    dump_bounds_pending = false;
    check_dump_window();
  };
  if (!dump_bounds_pending) {
    check_dump_window();
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
    if (dump_bounds_pending) {
      resolve_dump_bounds();
    }
    dump_at(context.time());
  }

  if (dump_bounds_pending) {
    cvm::log(cvm::ERROR,
             "Error: cycle-based dump plusargs need the clock table rv_tester registers from CLK_PROFILE_FREQ_MHZ; "
             "use the time-based (+*_dump_on/off) form instead\n");
    exit(1);
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
