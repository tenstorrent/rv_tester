#include <cinttypes>
#include <array>

namespace cla_mmr {

 #define CLA_MMR_REGS \
    X(TR_DST_CONTROL         ,0x42002000)\
    X(TR_DST_IMPL            ,0x42002004)\
    X(TR_DST_INSTFEATURES    ,0x42002008)\
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
    X(CDBG_NODE2_EAP0_CFG    ,0x42002140)\
    X(CDBG_TRACE_CFG         ,0x420021A0)\
    X(CDBG_NTRACE_CFG        ,0x420021A8)\


 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 CLA_MMR_REGS
 #undef X
 };

 #undef CLA_MMR_REGS
 }
