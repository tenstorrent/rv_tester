#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  constexpr uint32_t pll_ip_ver             = 0x210'3000;
  constexpr uint32_t pll_control            = 0x210'3004;
  constexpr uint32_t pll_status             = 0x210'3008;
  constexpr uint32_t pll_interrupts         = 0x210'300C;
  constexpr uint32_t pll_parameters0        = 0x210'3010;
  constexpr uint32_t pll_parameters1        = 0x210'3014;

  constexpr uint32_t cold_powerup_idx       = 4;
  constexpr uint32_t dfs_done_idx           = 0;
  constexpr uint32_t dfs_req_idx            = 0;
  constexpr uint32_t scalar_div_idx         = 16;
  constexpr uint32_t main_divider_div_idx   = 6;
  constexpr uint32_t pre_divider_div_idx    = 0;

  constexpr uint32_t rst_ctl_ip_ver         = 0x210'2000;
  constexpr uint32_t rst_ctl_cold           = 0x210'2004;
  constexpr uint32_t rst_ctl_warm           = 0x210'2008;
  constexpr uint32_t rst_ctl_nofetch        = 0x210'200C;

  constexpr uint32_t cpl_cl_cold_reset_n    = 0;
  constexpr uint32_t cpl_cl_warm_reset_n    = 0;
  constexpr uint32_t cpl_cl_no_fetch        = 0;

  constexpr uint32_t core_fuse_mmr          = 0x4200'FFF8;
  constexpr uint32_t trace_fuse_mmr         = 0x4208'FFF8;
  constexpr uint32_t aclint_fuse_mmr        = 0x4218'FFF8;
  constexpr uint32_t dm_fuse_mmr            = 0x4219'FFF8;
  constexpr uint32_t sc_fuse_mmr            = 0x421A'7FD8;
  constexpr uint32_t sw_fuse_mmr            = 0x421B'FFF8;
  constexpr uint32_t core_fuse_offset       = 0x0001'0000;

  constexpr uint32_t core_fuse_idx          = 16;
  constexpr uint32_t trace_fuse_idx         = 8;
  constexpr uint32_t dm_fuse_idx            = 9;
  constexpr uint32_t sc_fuse_idx            = 0;
  constexpr uint32_t lock_idx               = 15;

  constexpr uint32_t core_pversion_mmr          = 0x4200'5000;
  constexpr uint32_t core_pcontrol_mmr          = 0x4200'5040;
  constexpr uint32_t core_preg0_mmr             = 0x4200'5080;
  constexpr uint32_t core_preg1_mmr             = 0x4200'5088;
  constexpr uint32_t core_preg2_mmr             = 0x4200'5090;
  constexpr uint32_t core_preg3_mmr             = 0x4200'5098;
  constexpr uint32_t core_ptvec_csr             = 0x4200'3DA8;

  constexpr uint32_t smc_local_base             = 0x0210'0000;
  constexpr uint32_t cpl_sram_base              = smc_local_base +0x40000;
  constexpr uint32_t cpl_patch_ram_base         = cpl_sram_base +0x0c000;
  constexpr uint32_t cpl_patch_ram_ptrig_0     =  cpl_patch_ram_base + 0x0400;
  constexpr uint32_t cpl_patch_ram_ptrig_1     =  cpl_patch_ram_base + 0x0440;
  constexpr uint32_t cpl_patch_ram_ptrig_2     =  cpl_patch_ram_base + 0x0480;
  constexpr uint32_t cpl_patch_ram_ptrig_3     =  cpl_patch_ram_base + 0x04c0;
  constexpr uint32_t cpl_patch_ram_pbody_0     =  cpl_patch_ram_base + 0x0500;
  constexpr uint32_t cpl_patch_ram_pbody_1     =  cpl_patch_ram_base + 0x0900;
  constexpr uint32_t cpl_patch_ram_pbody_2     =  cpl_patch_ram_base + 0x0d00;
  constexpr uint32_t cpl_patch_ram_pbody_3     =  cpl_patch_ram_base + 0x01100;
  constexpr uint32_t cpl_patch_ram_pdata       =  cpl_patch_ram_base + 0x1600;


  typedef enum : bool { COLD = true, WARM = false } rst_t;
  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;

  std::vector<uint32_t> patch_header = {
    0x7c906ff3,        //0x4214c000 :    	csrrsi	t6,0x7c9,0
    0x002fff93,        //0x4214c004 :    	andi	t6,t6,2
    0x7c9f9073,        //0x4214c008 :    	csrw	0x7c9,t6
    0x7b209073,        //0x4214c00c :    	csrw	dscratch0,ra
    0x7b311073,        //0x4214c010 :    	csrw	dscratch1,sp
    0x7b506ff3,        //0x4214c014 :    	csrrsi	t6,0x7b5,0
    0x00cfd093,        //0x4214c018 :    	srli	ra,t6,0xc
    0x00c09093,        //0x4214c01c :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c020 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c024 :    	andi	sp,sp,7
    0x00811113,        //0x4214c028 :    	slli	sp,sp,0x8
    0x00001fb7,        //0x4214c02c :    	lui	t6,0x1
    0x600f8f9b,        //0x4214c030 :    	addiw	t6,t6,1536 # 1600 <tohost-0x6fffea00>
    0x00110133,        //0x4214c034 :    	add	sp,sp,ra
    0x01f10133,        //0x4214c038 :    	add	sp,sp,t6
    0x00312c23,        //0x4214c03c :    	sw	gp,24(sp)
    0x02412023,        //0x4214c040 :    	sw	tp,32(sp)
    0x02512423,        //0x4214c044 :    	sw	t0,40(sp)
    0x02612823,        //0x4214c048 :    	sw	t1,48(sp)
    0x02712c23,        //0x4214c04c :    	sw	t2,56(sp)
    0x04812023,        //0x4214c050 :    	sw	s0,64(sp)
    0x04912423,        //0x4214c054 :    	sw	s1,72(sp)
    0x04a12823,        //0x4214c058 :    	sw	a0,80(sp)
    0x04b12c23,        //0x4214c05c :    	sw	a1,88(sp)
    0x06c12023,        //0x4214c060 :    	sw	a2,96(sp)
    0x06d12423,        //0x4214c064 :    	sw	a3,104(sp)
    0x06e12823,        //0x4214c068 :    	sw	a4,112(sp)
    0x06f12c23,        //0x4214c06c :    	sw	a5,120(sp)
    0x09012023,        //0x4214c070 :    	sw	a6,128(sp)
    0x09112423,        //0x4214c074 :    	sw	a7,136(sp)
    0x09212823,        //0x4214c078 :    	sw	s2,144(sp)
    0x09312c23,        //0x4214c07c :    	sw	s3,152(sp)
    0x0b412023,        //0x4214c080 :    	sw	s4,160(sp)
    0x0b512423,        //0x4214c084 :    	sw	s5,168(sp)
    0x0b712823,        //0x4214c088 :    	sw	s7,176(sp)
    0x0b812c23,        //0x4214c08c :    	sw	s8,184(sp)
    0x0d912023,        //0x4214c090 :    	sw	s9,192(sp)
    0x0da12423,        //0x4214c094 :    	sw	s10,200(sp)
    0x0db12823,        //0x4214c098 :    	sw	s11,208(sp)
    0x0dc12c23,        //0x4214c09c :    	sw	t3,216(sp)
    0x0fd12023,        //0x4214c0a0 :    	sw	t4,224(sp)
    0x0fe12823,        //0x4214c0a4 :    	sw	t5,240(sp)
    0x0ff12c23,        //0x4214c0a8 :    	sw	t6,248(sp)
    0x7b206f73,        //0x4214c0ac :    	csrrsi	t5,dscratch0,0
    0x7b306ff3,        //0x4214c0b0 :    	csrrsi	t6,dscratch1,0
    0x01e12423,        //0x4214c0b4 :    	sw	t5,8(sp)
    0x01f12823,        //0x4214c0b8 :    	sw	t6,16(sp)
    0x7c8061f3,        //0x4214c0bc :    	csrrsi	gp,0x7c8,0
    0x7b006273,        //0x4214c0c0 :    	csrrsi	tp,dcsr,0
    0x1c027213,        //0x4214c0c4 :    	andi	tp,tp,448
    0x00625213,        //0x4214c0c8 :    	srli	tp,tp,0x6
    0x00621213,        //0x4214c0cc :    	slli	tp,tp,0x6
    0x00408233,        //0x4214c0d0 :    	add	tp,ra,tp
    0x40020213,        //0x4214c0d4 :    	addi	tp,tp,1024 # 400 <tohost-0x6ffffc00>
    0x7b106f73,        //0x4214c0d8 :    	csrrsi	t5,dpc,0
    0x00020067,        //0x4214c0dc :    	jr	tp # 0 <tohost-0x70000000>
    0x7c906ff3,        //0x4214c0e0 :    	csrrsi	t6,0x7c9,0
    0x002fff93,        //0x4214c0e4 :    	andi	t6,t6,2
    0x7c9f9073,        //0x4214c0e8 :    	csrw	0x7c9,t6
    0x7b509073,        //0x4214c0ec :    	csrw	0x7b5,ra
    0x00c0d093,        //0x4214c0f0 :    	srli	ra,ra,0xc
    0x00c09093,        //0x4214c0f4 :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c0f8 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c0fc :    	andi	sp,sp,7
    0x00811113,        //0x4214c100 :    	slli	sp,sp,0x8
    0x00110133,        //0x4214c104 :    	add	sp,sp,ra
    0x00001fb7,        //0x4214c108 :    	lui	t6,0x1
    0x600f8f9b,        //0x4214c10c :    	addiw	t6,t6,1536 # 1600 <tohost-0x6fffea00>
    0x01f10133,        //0x4214c110 :    	add	sp,sp,t6
    0x00812f03,        //0x4214c114 :    	lw	t5,8(sp)
    0x01012f83,        //0x4214c118 :    	lw	t6,16(sp)
    0x7b2f1073,        //0x4214c11c :    	csrw	dscratch0,t5
    0x7b3f9073,        //0x4214c120 :    	csrw	dscratch1,t6
    0x01812183,        //0x4214c124 :    	lw	gp,24(sp)
    0x02012203,        //0x4214c128 :    	lw	tp,32(sp)
    0x02812283,        //0x4214c12c :    	lw	t0,40(sp)
    0x03012303,        //0x4214c130 :    	lw	t1,48(sp)
    0x03812383,        //0x4214c134 :    	lw	t2,56(sp)
    0x04012403,        //0x4214c138 :    	lw	s0,64(sp)
    0x04812483,        //0x4214c13c :    	lw	s1,72(sp)
    0x05012503,        //0x4214c140 :    	lw	a0,80(sp)
    0x05812583,        //0x4214c144 :    	lw	a1,88(sp)
    0x06012603,        //0x4214c148 :    	lw	a2,96(sp)
    0x06812683,        //0x4214c14c :    	lw	a3,104(sp)
    0x07012703,        //0x4214c150 :    	lw	a4,112(sp)
    0x07812783,        //0x4214c154 :    	lw	a5,120(sp)
    0x08012803,        //0x4214c158 :    	lw	a6,128(sp)
    0x08812883,        //0x4214c15c :    	lw	a7,136(sp)
    0x09012903,        //0x4214c160 :    	lw	s2,144(sp)
    0x09812983,        //0x4214c164 :    	lw	s3,152(sp)
    0x0a012a03,        //0x4214c168 :    	lw	s4,160(sp)
    0x0a812a83,        //0x4214c16c :    	lw	s5,168(sp)
    0x0b012b03,        //0x4214c170 :    	lw	s6,176(sp)
    0x0b812b83,        //0x4214c174 :    	lw	s7,184(sp)
    0x0c012c03,        //0x4214c178 :    	lw	s8,192(sp)
    0x0c812c83,        //0x4214c17c :    	lw	s9,200(sp)
    0x0d012d03,        //0x4214c180 :    	lw	s10,208(sp)
    0x0d812d83,        //0x4214c184 :    	lw	s11,216(sp)
    0x0e012e03,        //0x4214c188 :    	lw	t3,224(sp)
    0x0e812e83,        //0x4214c18c :    	lw	t4,232(sp)
    0x0f012f03,        //0x4214c190 :    	lw	t5,240(sp)
    0x0f812f83,        //0x4214c194 :    	lw	t6,248(sp)
    0x7b2060f3,        //0x4214c198 :    	csrrsi	ra,dscratch0,0
    0x7b306173,        //0x4214c19c :    	csrrsi	sp,dscratch1,0
    0x7c906ff3,        //0x4214c1a0 :    	csrrsi	t6,0x7c9,0
    0x003fef93,        //0x4214c1a4 :    	ori	t6,t6,3
    0x7c9f9073,        //0x4214c1a4 :    	csrw	0x7c9,t6
    0x7b200073,        //0x4214c1a8 :    	dret
    0x00000013,        //0x4214c1ac :    	nop
    0x00000013,        //0x4214c1b0 :    	nop
  };
  std::vector<uint32_t> patch_trig_0 = {
    0x4214cfb7,        	//lui	t6,0x4214c
    0x500f8f9b,        	//addiw	t6,t6,1280 # 4214c500 <tohost-0x2deb3b00>
    0x000f8067,        	//jr	t6
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013          //nop
  };
  std::vector<uint32_t> patch_trig_1 = {
    0x4214dfb7,        	//lui	t6,0x4214d
    0x900f8f9b,        	//addiw	t6,t6,-1792 # 4214c900 <tohost-0x2deb3700>
    0x000f8067,        	//jr	t6
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
  };
  std::vector<uint32_t> patch_trig_2 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0xd00f8f9b,         //addiw	t6,t6,-768 # 4214cd00 <tohost-0x2deb3300>
    0x000f8067,         //jr	t6
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
  };
  std::vector<uint32_t> patch_trig_3 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0x100f8f9b,         //addiw	t6,t6,256 # 4214d100 <tohost-0x2deb2f00>
    0x000f8067,         //jr	t6
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
  };
  std::vector<uint32_t> patch_body_sub = {
    0x00f1d213,          	//srli	tp,gp,0xf
    0x01f27213,          	//andi	tp,tp,31
    0x0141d293,          	//srli	t0,gp,0x14
    0x01f2f293,          	//andi	t0,t0,31
    0x0071d313,          	//srli	t1,gp,0x7
    0x01f37313,          	//andi	t1,t1,31
    0x00321213,          	//slli	tp,tp,0x3
    0x00220233,          	//add	tp,tp,sp
    0x00023203,          	//ld	tp,0(tp) # 0 <tohost-0x70000000>
    0x00329293,          	//slli	t0,t0,0x3
    0x002282b3,          	//add	t0,t0,sp
    0x0002b283,          	//ld	t0,0(t0)
    0x00300f93,          	//li	t6,3
    0x7c9f9073,          	//csrw	0x7c9,t6
    0xfff2c213,          	//not	tp,t0
    0x00100f93,          	//li	t6,1
    0x01f20233,          	//add	tp,tp,t6
    0x00331313,          	//slli	t1,t1,0x3
    0x00230333,          	//add	t1,t1,sp
    0x00432023,          	//sw	tp,0(t1)
    0x004f0093,          	//addi	ra,t5,4
    0x4214cfb7,          	//lui	t6,0x4214c
    0x0e0f8f9b,          	//addiw	t6,t6,224 # 4214c0e0 <tohost-0x2deb3f20>
    0x000f8067,          	//jr	t6
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
    0x00000013,           //nop
  };
  std::vector<uint32_t> patch_body_subw = {
    0x7b106ff3,         //csrrsi	t6,dpc,0
    0x004f8f93,         //addi	t6,t6,4
    0x41498933,         //sub	s2,s3,s4
    0x7b1f9073,         //csrw	dpc,t6
    0x00000f93,         //li	t6,0
    0x7b200073,         //dret
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
    0x00000013,         //nop
  };
}

class pwrmgmt {

  public:

    pwrmgmt(cvm::topology::loc_t, unsigned) {}
    ~pwrmgmt() {}

};
