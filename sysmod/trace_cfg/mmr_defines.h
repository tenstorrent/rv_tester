#include <cinttypes>
#include <array>

 namespace mmr {

 #define MMR_REGS \
    X(CDBG_NODE2_EAP1_CFG1        ,0x42002148)\
    X(CDBG_NODE2_EAP0_CFG1        ,0x42002140)\
    X(CDBG_NODE3_EAP0_CFG1        ,0x42002150)\
    X(CDBG_NODE3_EAP1_CFG1        ,0x42002158)\
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