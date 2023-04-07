#include <cstdint>

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val);
  void sysmod_terminate(uint8_t call_finish);
}