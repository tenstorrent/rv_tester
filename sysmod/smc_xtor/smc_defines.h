#include <cinttypes>
#include <array>

namespace smc_base {
// 0xC000_0000 will be added by SMC RTL hence removing that from BASE Address
#define SMC_BASES \
  X(SMC_LOCAL_BASE       ,0x02100000)\
  X(SMC_GLOBAL_BASE      ,0x00000000)\


 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 SMC_BASES
 #undef X
 };

  #undef SMC_BASES
 }

namespace smc_mmr_base {

#define SMC_MMR_BASE_ADDRS \
  X(CPL_INFW_BASE        ,smc_base::SMC_LOCAL_BASE + 0x015000)\
  X(CPL_OUTFW_BASE       ,smc_base::SMC_LOCAL_BASE + 0x016000)\
  X(CPL_SRAM_BASE        ,smc_base::SMC_LOCAL_BASE + 0x040000)\

 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 SMC_MMR_BASE_ADDRS
 #undef X
 };

  #undef SMC_MMR_BASE_ADDRS
}

namespace smc_addr {

#define SMC_ADDRS \
  X(CPL_INFW_CFG_00      ,smc_mmr_base::CPL_INFW_BASE + 0x00000)\
  X(CPL_INFW_CFG_01      ,smc_mmr_base::CPL_INFW_BASE + 0x00004)\
  X(CPL_INFW_CFG_02      ,smc_mmr_base::CPL_INFW_BASE + 0x00008)\
  X(CPL_INFW_CFG_03      ,smc_mmr_base::CPL_INFW_BASE + 0x0000C)\
  X(CPL_INFW_CFG_04      ,smc_mmr_base::CPL_INFW_BASE + 0x00010)\
  X(CPL_INFW_CFG_05      ,smc_mmr_base::CPL_INFW_BASE + 0x00014)\
  X(CPL_INFW_CFG_06      ,smc_mmr_base::CPL_INFW_BASE + 0x00018)\
  X(CPL_INFW_CFG_07      ,smc_mmr_base::CPL_INFW_BASE + 0x0001C)\
  X(CPL_INFW_CFG_08      ,smc_mmr_base::CPL_INFW_BASE + 0x00020)\
  X(CPL_INFW_CFG_09      ,smc_mmr_base::CPL_INFW_BASE + 0x00024)\
  X(CPL_INFW_CFG_10      ,smc_mmr_base::CPL_INFW_BASE + 0x00028)\
  X(CPL_INFW_CFG_11      ,smc_mmr_base::CPL_INFW_BASE + 0x0002C)\
  X(CPL_INFW_CFG_12      ,smc_mmr_base::CPL_INFW_BASE + 0x00030)\
  X(CPL_INFW_CFG_13      ,smc_mmr_base::CPL_INFW_BASE + 0x00034)\
  X(CPL_INFW_CFG_14      ,smc_mmr_base::CPL_INFW_BASE + 0x00038)\
  X(CPL_INFW_CFG_15      ,smc_mmr_base::CPL_INFW_BASE + 0x0003C)\
  X(CPL_INFW_ADDRL_00    ,smc_mmr_base::CPL_INFW_BASE + 0x00040)\
  X(CPL_INFW_ADDRH_00    ,smc_mmr_base::CPL_INFW_BASE + 0x00044)\
  X(CPL_INFW_ADDRL_01    ,smc_mmr_base::CPL_INFW_BASE + 0x00048)\
  X(CPL_INFW_ADDRH_01    ,smc_mmr_base::CPL_INFW_BASE + 0x0004C)\
  X(CPL_INFW_ADDRL_02    ,smc_mmr_base::CPL_INFW_BASE + 0x00050)\
  X(CPL_INFW_ADDRH_02    ,smc_mmr_base::CPL_INFW_BASE + 0x00054)\
  X(CPL_INFW_ADDRL_03    ,smc_mmr_base::CPL_INFW_BASE + 0x00058)\
  X(CPL_INFW_ADDRH_03    ,smc_mmr_base::CPL_INFW_BASE + 0x0005C)\
  X(CPL_INFW_ADDRL_04    ,smc_mmr_base::CPL_INFW_BASE + 0x00060)\
  X(CPL_INFW_ADDRH_04    ,smc_mmr_base::CPL_INFW_BASE + 0x00064)\
  X(CPL_INFW_ADDRL_05    ,smc_mmr_base::CPL_INFW_BASE + 0x00068)\
  X(CPL_INFW_ADDRH_05    ,smc_mmr_base::CPL_INFW_BASE + 0x0006C)\
  X(CPL_INFW_ADDRL_06    ,smc_mmr_base::CPL_INFW_BASE + 0x00070)\
  X(CPL_INFW_ADDRH_06    ,smc_mmr_base::CPL_INFW_BASE + 0x00074)\
  X(CPL_INFW_ADDRL_07    ,smc_mmr_base::CPL_INFW_BASE + 0x00078)\
  X(CPL_INFW_ADDRH_07    ,smc_mmr_base::CPL_INFW_BASE + 0x0007C)\
  X(CPL_INFW_ADDRL_08    ,smc_mmr_base::CPL_INFW_BASE + 0x00080)\
  X(CPL_INFW_ADDRH_08    ,smc_mmr_base::CPL_INFW_BASE + 0x00084)\
  X(CPL_INFW_ADDRL_09    ,smc_mmr_base::CPL_INFW_BASE + 0x00088)\
  X(CPL_INFW_ADDRH_09    ,smc_mmr_base::CPL_INFW_BASE + 0x0008C)\
  X(CPL_INFW_ADDRL_10    ,smc_mmr_base::CPL_INFW_BASE + 0x00090)\
  X(CPL_INFW_ADDRH_10    ,smc_mmr_base::CPL_INFW_BASE + 0x00094)\
  X(CPL_INFW_ADDRL_11    ,smc_mmr_base::CPL_INFW_BASE + 0x00098)\
  X(CPL_INFW_ADDRH_11    ,smc_mmr_base::CPL_INFW_BASE + 0x0009C)\
  X(CPL_INFW_ADDRL_12    ,smc_mmr_base::CPL_INFW_BASE + 0x000A0)\
  X(CPL_INFW_ADDRH_12    ,smc_mmr_base::CPL_INFW_BASE + 0x000A4)\
  X(CPL_INFW_ADDRL_13    ,smc_mmr_base::CPL_INFW_BASE + 0x000A8)\
  X(CPL_INFW_ADDRH_13    ,smc_mmr_base::CPL_INFW_BASE + 0x000AC)\
  X(CPL_INFW_ADDRL_14    ,smc_mmr_base::CPL_INFW_BASE + 0x000B0)\
  X(CPL_INFW_ADDRH_14    ,smc_mmr_base::CPL_INFW_BASE + 0x000B4)\
  X(CPL_INFW_ADDRL_15    ,smc_mmr_base::CPL_INFW_BASE + 0x000B8)\
  X(CPL_INFW_ADDRH_15    ,smc_mmr_base::CPL_INFW_BASE + 0x000BC)\
  X(CPL_OUTFW_CFG_00     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00000)\
  X(CPL_OUTFW_CFG_01     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00004)\
  X(CPL_OUTFW_CFG_02     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00008)\
  X(CPL_OUTFW_CFG_03     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0000C)\
  X(CPL_OUTFW_CFG_04     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00010)\
  X(CPL_OUTFW_CFG_05     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00014)\
  X(CPL_OUTFW_CFG_06     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00018)\
  X(CPL_OUTFW_CFG_07     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0001C)\
  X(CPL_OUTFW_CFG_08     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00020)\
  X(CPL_OUTFW_CFG_09     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00024)\
  X(CPL_OUTFW_CFG_10     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00028)\
  X(CPL_OUTFW_CFG_11     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0002C)\
  X(CPL_OUTFW_CFG_12     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00030)\
  X(CPL_OUTFW_CFG_13     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00034)\
  X(CPL_OUTFW_CFG_14     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00038)\
  X(CPL_OUTFW_CFG_15     ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0003C)\
  X(CPL_OUTFW_ADDRL_00   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00040)\
  X(CPL_OUTFW_ADDRH_00   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00044)\
  X(CPL_OUTFW_ADDRL_01   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00048)\
  X(CPL_OUTFW_ADDRH_01   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0004C)\
  X(CPL_OUTFW_ADDRL_02   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00050)\
  X(CPL_OUTFW_ADDRH_02   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00054)\
  X(CPL_OUTFW_ADDRL_03   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00058)\
  X(CPL_OUTFW_ADDRH_03   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0005C)\
  X(CPL_OUTFW_ADDRL_04   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00060)\
  X(CPL_OUTFW_ADDRH_04   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00064)\
  X(CPL_OUTFW_ADDRL_05   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00068)\
  X(CPL_OUTFW_ADDRH_05   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0006C)\
  X(CPL_OUTFW_ADDRL_06   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00070)\
  X(CPL_OUTFW_ADDRH_06   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00074)\
  X(CPL_OUTFW_ADDRL_07   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00078)\
  X(CPL_OUTFW_ADDRH_07   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0007C)\
  X(CPL_OUTFW_ADDRL_08   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00080)\
  X(CPL_OUTFW_ADDRH_08   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00084)\
  X(CPL_OUTFW_ADDRL_09   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00088)\
  X(CPL_OUTFW_ADDRH_09   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0008C)\
  X(CPL_OUTFW_ADDRL_10   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00090)\
  X(CPL_OUTFW_ADDRH_10   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00094)\
  X(CPL_OUTFW_ADDRL_11   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x00098)\
  X(CPL_OUTFW_ADDRH_11   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x0009C)\
  X(CPL_OUTFW_ADDRL_12   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000A0)\
  X(CPL_OUTFW_ADDRH_12   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000A4)\
  X(CPL_OUTFW_ADDRL_13   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000A8)\
  X(CPL_OUTFW_ADDRH_13   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000AC)\
  X(CPL_OUTFW_ADDRL_14   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000B0)\
  X(CPL_OUTFW_ADDRH_14   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000B4)\
  X(CPL_OUTFW_ADDRL_15   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000B8)\
  X(CPL_OUTFW_ADDRH_15   ,smc_mmr_base::CPL_OUTFW_BASE  + 0x000BC)\

 enum reg : uint32_t {
 #define X(name, value) \
   name = value,
 SMC_ADDRS
 #undef X
 };


struct r { const char* name; std::uint32_t value; };

constexpr std::array list = { 
#define X(name, value) \
    r{#name, value},
    SMC_ADDRS
#undef X
};

 #undef SMC_ADDRS
 }
