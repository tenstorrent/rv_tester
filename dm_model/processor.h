

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cassert>

// architectural state of a RISC-V hart
struct state_t
{ 
  bool debug_mode;
  uint8_t debug_entry_cause;

  enum {
      STEP_NONE,
      STEP_STEPPING,
      STEP_STEPPED
  } single_step;

};

// this class represents one processor in a RISC-V machine.
class processor_t
{
public:
  processor_t(bool halt_on_reset); // because of command line option --log and -s we need both
  ~processor_t();

  void set_debug(bool value);
  void reset();
  // void step(size_t n); // run for n cycles
  state_t* get_state() { return &state; }

  bool debug;

  bool halted() { return state.debug_mode; }

  enum {
    HR_NONE,    /* Halt request is inactive. */
    HR_REGULAR, /* Regular halt request/debug interrupt. */
    HR_GROUP    /* Halt requested due to halt group. */
  } halt_request;

private:
  state_t state;
  bool halt_on_reset;

  void enter_debug_mode(uint8_t cause);
};

processor_t::processor_t(bool halt_on_reset)
  : debug(false), halt_request(HR_NONE), halt_on_reset(halt_on_reset)
{
  reset();
}

processor_t::~processor_t() {}

void processor_t::set_debug(bool value)
{
  debug = value;
}

void processor_t::reset()
{
  state.debug_mode = false;
  state.debug_entry_cause = 0; //Check what value to keep on reset
  state.single_step = state_t::STEP_NONE;

  debug = false;
  halt_request = HR_NONE;
  halt_on_reset = false;
}

void processor_t::enter_debug_mode(uint8_t cause)
{
  state.debug_mode = true;
  state.debug_entry_cause = cause;
  // Add the logic for the single_step
}