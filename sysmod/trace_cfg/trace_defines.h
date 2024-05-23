#include <cinttypes>
#include <array>

 namespace mmr {

 #define MMR_REGS \
    X(CDBG_NODE2_EAP1_CFG         ,0x42002148)\
    X(CDBG_NODE2_EAP0_CFG         ,0x42002140)\
    X(CDBG_NODE3_EAP0_CFG         ,0x42002150)\
    X(CDBG_NODE3_EAP1_CFG         ,0x42002158)\
    X(CDBG_SIGNAL_MASK0           ,0x42002160)\
    X(CDBG_SIGNAL_MATCH0          ,0x42002168)\
    X(CDBG_SIGNAL_MASK1           ,0x42002170)\
    X(CDBG_SIGNAL_MATCH1          ,0x42002178)\
    X(C_DBG_ONES_COUNT_VALUE      ,0x420021D0)\
    X(C_DBG_ONES_COUNT_MASK       ,0x420021C8)\
    X(C_DBG_TRANSITION_TO_VALUE   ,0x420021C0)\
    X(C_DBG_TRANSITION_FROM_VALUE ,0x420021B8)\
    X(C_DBG_TRANSITION_MASK       ,0x420021B0)\
    X(C_DBG_MUX_SEL               ,0x42002198)\

 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 MMR_REGS
 #undef X
 };

 struct r { const char* name; std::uint32_t value; };

 constexpr std::array list = {
 #define X(name, value) \
   r{#name, value},
   MMR_REGS
 #undef X
 };

 #undef MMR_REGS
 }


namespace trace_mmr {

 #define TRACE_MMR_REGS \
    X(TR_FUNNEL_CONTROL      ,0x42081000)\
    X(TR_RAM_CONTROL         ,0x42080000)\
    X(TR_RAM_START_LOW       ,0x42080010)\
    X(TR_RAM_START_HIGH      ,0x42080014)\
    X(TR_RAM_LIMIT_LOW       ,0x42080018)\
    X(TR_RAM_LIMIT_HIGH      ,0x4208001C)\
    X(TR_RAM_WP_LOW          ,0x42080020)\
    X(TR_RAM_WP_HIGH         ,0x42080024)\
    X(TR_RAM_RP_LOW          ,0x42080028)\
    X(TR_RAM_RP_HIGH         ,0x4208002C)\
    X(TR_RAM_DATA            ,0x42080040)\
    X(TR_DST_RAM_CONTROL     ,0x42082000)\
    X(TR_DST_RAM_START_LOW   ,0x42082010)\
    X(TR_DST_RAM_START_HIGH  ,0x42082014)\
    X(TR_DST_RAM_LIMIT_LOW   ,0x42082018)\
    X(TR_DST_RAM_LIMIT_HIGH  ,0x4208201C)\
    X(TR_DST_RAM_WP_LOW      ,0x42082020)\
    X(TR_DST_RAM_WP_HIGH     ,0x42082024)\
    X(TR_DST_RAM_RP_LOW      ,0x42082028)\
    X(TR_DST_RAM_RP_HIGH     ,0x4208202C)\
    X(TR_DST_RAM_DATA        ,0x42082040)\
    X(TR_DST_CONTROL         ,0x42002000)\
    X(TR_DST_IMPL            ,0x42002004)\
    X(CDBG_CLA_CTRL_STS_CFG  ,0x42002190)\
    X(CDBG_MUX_SEL_CFG       ,0x42002198)\
    X(CDBG_CLA_COUNTER0_CFG  ,0x42002100)\
    X(CDBG_CLA_COUNTER1_CFG  ,0x42002108)\
    X(CDBG_CLA_COUNTER2_CFG  ,0x42002110)\
    X(CDBG_CLA_COUNTER3_CFG  ,0x42002118)\
    X(CDBG_NODE0_EAP0_CFG    ,0x42002120)\
    X(CDBG_NODE0_EAP1_CFG    ,0x42002128)\
    X(CDBG_NODE1_EAP0_CFG    ,0x42002130)\
    X(CDBG_NODE1_EAP1_CFG    ,0x42002138)\
    X(CDBG_TRACE_CFG         ,0x420021A0)\
    X(CDBG_NTRACE_CFG        ,0x420021A8)\
    X(CDBG_MUX_SEL_EXT_CFG   ,0x42002220)\


 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 TRACE_MMR_REGS
 #undef X
 };

 #undef TRACE_MMR_REGS
 }
