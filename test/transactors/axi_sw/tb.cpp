// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <cinttypes>
#include <cstdio>
#include <cassert>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"

#include "transactor.h"

#include "rv_tester_transactions.hpp"

DEFINE_bool(covg_disable_cg, false, "vcs cov arg");
DEFINE_int32(cg_coverage_control, false, "vcs cov arg");
DEFINE_string(cm_dir, "", "vcs cov arg");
DEFINE_string(log, "", "run.log");

DEFINE_bool(UVM_DISABLE_AUTO_COMPONENT, false, "UVM_DISABLE_AUTO_COMPONENT");
DEFINE_bool(UVM_TR_RECORD, false, "UVM_TR_RECORD");
DEFINE_string(fsdb_all, "", "fsdb+all");
DEFINE_bool(vcs_initreg_0, false, "vcs+initreg+0");
DEFINE_int64(fsdb_dump_on, 0, "fsdb_dump_on");
DEFINE_int64(fsdb_dump_off, 0, "fsdb_dump_off");
DEFINE_int64(fsdb_cycle_on, 0, "fsdb_cycle_on");
DEFINE_int64(fsdb_cycle_off, 0, "fsdb_cycle_off");
DEFINE_string(UVM_VERDI_TRACE, "", "UVM_VERDI_TRACE");
DEFINE_string(fsdb_gate, "", "fsdb_gate");
DEFINE_bool(l, false, "l");
DEFINE_string(sml, "", "sml");
DEFINE_bool(ucli, false, "ucli");
DEFINE_bool(ucli2Proc, false, "ucli2Proc");

extern "C" void get_stim(
    std::uint32_t clocks,
    std::uint8_t* finish,
    std::uint8_t* reset_n,
    std::uint8_t* tb_reset,
    std::uint8_t* aw_valid,
    std::uint64_t* aw_addr,
    std::uint8_t* aw_len,
    std::uint8_t* aw_size,
    std::uint8_t* aw_burst,
    std::uint8_t* aw_atop,
    std::uint8_t* aw_lock,
    std::uint8_t* ar_valid,
    std::uint8_t* ar_id,
    std::uint64_t* ar_addr,
    std::uint8_t* ar_len,
    std::uint8_t* ar_size,
    std::uint8_t* ar_burst,
    std::uint8_t* ar_lock,
    std::uint8_t* w_valid,
    std::uint8_t w_data[64],
    std::uint8_t w_strb[8],
    std::uint8_t* w_last,
    std::uint8_t r_valid,
    std::uint8_t r_id,
    std::uint8_t r_data[64],
    std::uint8_t r_last) {

  enum state_t {
    IDLE,
    WRITE_ADDR,
    WRITE_DATA,
    READ_ADDR,
    READ_DATA,
  };

  static state_t state;
  static int i;

  *reset_n = clocks > 15;
  *tb_reset = clocks == 1;
  *finish = 0;

  *aw_valid = 0;
  *w_valid = 0;
  *ar_valid = 0;

  if (!*reset_n) {
    state = IDLE;
    i = 0;
  }

  const int iterations = 1;

  switch (state) {
  case (IDLE):
    if (clocks > 20)
      state = WRITE_ADDR;
    break;
  case (WRITE_ADDR):
    *aw_valid = 1;
    *aw_addr = 0xabcd00;
    *aw_len = 0;
    *aw_size = 6;
    *aw_burst = 0;
    *aw_atop = 0;
    *aw_lock = 0;
    state = WRITE_DATA;
    break;
  case (WRITE_DATA):
    *w_valid = 1;
    w_data[0] = 0xee;
    w_data[63] = 0xaa;
    for (int i = 1; i < 63; i++)
      w_data[i] = i;
    w_strb[0] = 0xff;
    w_strb[7] = 0xff;
    for (int i = 1; i < 7; i++)
      w_strb[i] = 0;
    *w_last = 1;
    state = READ_ADDR;
    break;
  case (READ_ADDR):
    *ar_valid = 1;
    *ar_id = 3;
    *ar_addr = 0xabcd00;
    *ar_len = 0;
    *ar_size = 6;
    *ar_burst = 0;
    *ar_lock = 0;
    state = READ_DATA;
    break;
  case (READ_DATA):
    if (r_valid) {
      if (!(i % 1000))
        printf("%d tb received read data %" PRIx8 "..%" PRIx8 "\n", i, r_data[0], r_data[63]);
      assert(r_last);
      assert(r_id == 3);
      assert(r_data[0] == 0xee);
      assert(r_data[63] == 0xaa);
      for (int i = 1; i < 63; i++)
        assert(r_data[i] == ((i < 8 || i >= 56) ? i : 0));
      if (++i >= iterations) {
        *finish = 1;
      } else {
        state = IDLE;
      }
    }
    break;
  }
}

extern "C" std::uint8_t axi_sw_tb_init() {
  static std::unordered_map<std::uint64_t, std::uint8_t> byte_map;

  cvm::plusargs::parse();
  cvm::registry::build();
  cvm::registry::configure();

  byte_map.clear();

  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");
  for (const auto& source : sources) {
    cvm::registry::messenger.connect<transactor::write_t>(
        source,
        [](const auto& w) {
          for (std::uint64_t i = 0; i < w.length; i++)
            if (w.strb[i])
              byte_map[w.addr + i] = w.data[i];
        });
    cvm::registry::messenger.connect<transactor::read_t>(
        source,
        [source](const auto& r) {
          std::vector<std::uint8_t> data(r.length);
          for (std::uint64_t i = 0; i < r.length; i++)
            data[i] = byte_map[r.addr + i];
          cvm::registry::messenger.signal(source, transactor::read_response_t{r.id, std::move(data)});
        });
  }

  return true;
}

extern "C" std::uint8_t axi_sw_tb_shutdown() {
  return cvm::registry::shutdown();
}

extern "C" std::uint8_t axi_sw_tb_flush_callbacks() {
  cvm::registry::callbacks.flush();
  return true;
}
