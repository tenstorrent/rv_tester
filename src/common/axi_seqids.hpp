#pragma once

#include <cstdint>

namespace {
    constexpr uint32_t seqid_width_           =  0x3;

    constexpr uint32_t RESET_SEQ_ID           =  0x1;
    constexpr uint32_t MMR_SEQ_ID             =  0x2;
    constexpr uint32_t CLA_SEQ_ID             =  0x3;
    constexpr uint32_t DST_TRACE_SEQ_ID       =  0x4;
    constexpr uint32_t SNOOP_GEN_SEQ_ID       =  0x5;
    constexpr uint32_t SCRATCHPAD_MEM_SEQ_ID  =  0x6;
    constexpr uint32_t THUB_SEQ_ID            =  0x7;
}
