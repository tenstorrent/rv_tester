#include <cstdio>
#include <limits>
#include <map>
#include <numeric>
#include <string>

#include "Vtop.h"
#include "verilated.h"

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

#include "cvm/logger.hpp"
#include "cvm/registry.hpp"

int main(int argc, char** argv) {
  printf("[main] starting\n");

  //------------------------------------------------------------
  // Create simulation context and model
  VerilatedContext context;
  context.threads(1);
  context.commandArgs(argc, argv);
  // Passing the empty string as instance name suppresses tracing
  // of the top level wrapper, which is what we want to match VCS.
  Vtop top{&context, ""};
  // Simulation timescale
  const int timeunit = context.timeunit();           // between 0 (1s) .. -15 (1fs)
  const int timeprecision = context.timeprecision(); // between 0 (1s) .. -15 (1fs) <= timeunit
  // timeunit -> timeprecision factor
  const uint64_t timeUnit2Prec = static_cast<uint64_t>(std::pow(10.0, -timeprecision - -timeunit));

  //------------------------------------------------------------
  // Clock configuration

  // Figure out clock half periods
  const auto loc = cvm::topology::get_from_type("CLKI", 0);
  const size_t nclks = static_cast<size_t>(cvm::topology::attr(loc, "NCLKS").second);
  const std::vector<uint32_t> freq_mhz = cvm::topology::list_attr(loc, "CLOCK_FREQ_MHZ").second;
  std::vector<uint64_t> half_period(nclks, 0);
  std::vector<uint64_t> toggles(nclks, 0);
  // Numerator to compute period for a MHz frequency at the current timeprecision
  const uint64_t mhz2periodNumer = static_cast<uint64_t>(std::pow(10.0, -timeprecision - 6));
  for (size_t i = 0; i < nclks; ++i) {
    half_period[i] = mhz2periodNumer / (2ULL * freq_mhz[i]);
    if (!half_period[i]) {
      cvm::log(cvm::ERROR, "Error: Insufficient timeprecision 1e{} for {}MHz clock\n", timeprecision, freq_mhz[i]);
      exit(1);
    }
  }
  const size_t core_clk_idx = cvm::topology::attr(loc, "CORE_CLK_IDX").second;
  const size_t tb_clk_idx = cvm::topology::attr(loc, "TB_CLK_IDX").second;
  const uint64_t tb_clk_period = 2 * half_period[tb_clk_idx];

  //------------------------------------------------------------
  // Dump configuration

  // Figure out when to turn on dumping
  bool dumping = true;
  uint64_t dump_on = 0;
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_on=")) {
    dump_on = std::stoull(valp) * tb_clk_period;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_on=")) {
    dump_on = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_on=")) {
    // Accept vcd_cycle_on for backward compatibility
    dump_on = std::stoull(valp) * tb_clk_period;
  } else {
    dumping = false;
  }

  // Figure out when to turn off dumping
  uint64_t dump_off = std::numeric_limits<uint64_t>::max();
  if (const char* const valp = vl_mc_scan_plusargs("fsdb_cycle_off=")) {
    dump_off = std::stoull(valp) * tb_clk_period;
  } else if (const char* const valp = vl_mc_scan_plusargs("fsdb_dump_off=")) {
    dump_off = std::stoull(valp) * timeUnit2Prec;
  } else if (const char* const valp = vl_mc_scan_plusargs("vcd_cycle_off=")) {
    // Accept vcd_cycle_off for backward compatibility
    dump_off = std::stoull(valp) * tb_clk_period;
  }

  // Make sure we are not doing something stupid, the time queue assumes this
  if (dump_on >= dump_off) {
    cvm::log(cvm::ERROR, "Error: dump on time ({}) must be before dump off time ({})\n", dump_on, dump_off);
    exit(1);
  }

#ifndef VERILATOR_WAVES
  if (dumping) {
    cvm::log(cvm::LOW, "Warning: wave dump requested but model compiled without dumping capability\n");
  }
#endif

  //------------------------------------------------------------
  // Event queue setup

  // The event queue: indexed by time, contains functions that take the
  // current simulation time, and return the next simulation time to
  // reschedule them. If a function returns the current, or earlier time,
  // it is not rescheduled.
  std::multimap<uint64_t, std::function<uint64_t(uint64_t)>> event_queue;

  // Need an event at time 0 for proper startup
  event_queue.emplace(0, [&](uint64_t) {
    return 0; // Do not re-schedule
  });

  // Schedule dump enable/disable
#ifdef VERILATOR_WAVES
  std::unique_ptr<VltTraceFile> tfp; // The dump file
  if (dumping) {
    context.traceEverOn(true);
    tfp.reset(new VltTraceFile{});
    // At time 'dump_on', open trace file and perform initial dump
    event_queue.emplace(dump_on, [&](uint64_t time) {
      // Verilator misfeature: partition instances are only created and
      // added to the context after the first evaluation, so must register
      // the trace callbacks after the first 'eval', but before the first
      // 'dump'. So let's just do it right before the first dump then ...
      context.trace(tfp.get(), 99, 0);
      tfp->open(TRACE_FILE_NAME);
      tfp->dump(time);
      return 0; // Do not reschedule
    });
    // At time 'dump_off', flush and close trace file
    event_queue.emplace(dump_off, [&](uint64_t time) {
      tfp->dump(time);
      tfp->flush();
      tfp->close();
      return 0; // Do not reschedule
    });
  }
#endif

  // Flag signaling a model input has changes
  bool input_changed = false;

  // Schedule clocks
  for (unsigned i = 0; i < nclks; ++i) {
    // Clock starts at zero
    top.clk_ext[i] = 0;
    // Toggles each half period
    event_queue.emplace(half_period[i], [&, i](uint64_t time) {
      top.clk_ext[i] = !top.clk_ext[i];
      ++toggles[i];
      input_changed = true;
      return time + half_period[i]; // Reschedule one half period later
    });
  }

  // Evaluate model at given time slot
  const auto evaluate = [&](uint64_t time) -> void {
    context.time(time);
    // Evaluate the model
    top.eval();
    // Schedule next time slot of internal event queue
    if (top.eventsPending()) {
      const uint64_t nextTimeSlot = top.nextTimeSlot();
      if (nextTimeSlot <= time) {
        cvm::log(cvm::ERROR, "Error: Event pending for same or earlier time slot\n");
        exit(1);
      }
      event_queue.emplace(nextTimeSlot, [&](uint64_t) {
        input_changed = true;
        return 0; // Do not reschedule
      });
    }
  };

  //------------------------------------------------------------
  // Simulation loop

  // Initial eval at time 0
  evaluate(0);

  while (!context.gotFinish()) {
    // Process next time step in the event queue
    const uint64_t time = event_queue.begin()->first;
    while (true) {
      auto it = event_queue.begin();
      // Stop when no more event at current time
      if (it->first > time)
        break;
      // Extract and run head of event queue
      auto node = event_queue.extract(it);
      const uint64_t next_time = node.mapped()(time);
      // Drop it if not scheduled in the future
      if (next_time <= time)
        continue;
      // Otherwise reschedule at the requested time
      node.key() = next_time;
      event_queue.insert(std::move(node));
    }

    // Evaluate the model if any inputs changed
    if (input_changed) {
      input_changed = false;
      // Already called eval at time 0, shouldn't need to do it again
      if (!time) {
        cvm::log(cvm::ERROR, "Error: multiple evaluations at time 0\n");
        exit(1);
      }
      // Evaluate model
      evaluate(time);
#ifdef VERILATOR_WAVES
      // Dump waveforms if dumping in progress
      if (tfp && tfp->isOpen())
        tfp->dump(time);
#endif
    }
  }

  // Run 'final' blocks
  top.final();

  //------------------------------------------------------------
  // Shutdown

#ifdef VERILATOR_WAVES
  // Close the waveform dump if finished before dump_off
  if (tfp && tfp->isOpen()) {
    tfp->flush();
    tfp->close();
  }
#endif

  printf("[main] exiting after %lu core cycles\n", toggles[core_clk_idx] / 2);
  exit(0);
}

extern "C" void assert_on_dpi() {
  Verilated::assertOn(true);
}

extern "C" void assert_off_dpi() {
  Verilated::assertOn(false);
}
