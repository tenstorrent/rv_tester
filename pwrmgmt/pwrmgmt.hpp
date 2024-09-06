#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <map>

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
  constexpr uint32_t exp_ctrl_fuse_idx      = 11;
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
  
  constexpr uint32_t core_ptvec_csr             = 0x0000'07CA;
  constexpr uint32_t core_pwr_throttle_cfg_0    = 0x0000'0BC6;
  constexpr uint32_t core_pwr_throttle_cfg_1    = 0x0000'0BC7;
  
  constexpr uint32_t core_crCsrCommandPort      = 0x4200'4008;
  constexpr uint32_t core_crCsrDataPort         = 0x4200'4000;


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
  constexpr uint32_t cpl_patch_ram_pbody_3     =  cpl_patch_ram_base + 0x1100;
  constexpr uint32_t cpl_patch_ram_pdata       =  cpl_patch_ram_base + 0x1500;
  constexpr uint32_t cpl_in_filter_addr_l      =  smc_local_base + 0x15008;
  constexpr uint32_t cpl_in_filter_addr_h      =  smc_local_base + 0x15010;
  constexpr uint32_t cpl_in_filter_config      =  smc_local_base + 0x15000;
  constexpr uint32_t cpl_out_filter_addr_l     =  smc_local_base + 0x16008;
  constexpr uint32_t cpl_out_filter_addr_h     =  smc_local_base + 0x16010;
  constexpr uint32_t cpl_out_filter_config     =  smc_local_base + 0x16000;
 

  constexpr uint32_t pm_mbox_base          =  0x0217'0000;
  constexpr uint32_t pm_mbox_reg           =  pm_mbox_base + 0x08;
  constexpr uint32_t pm_mbox_regdata       =  pm_mbox_base + 0x10;

  constexpr uint8_t thub_reg_base           =  0x0;
  constexpr uint8_t thub_threhold_param_reg =  thub_reg_base + 0x50;


  typedef enum : bool { COLD = true, WARM = false } rst_t;
  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
  typedef enum : bool { WR = 1, RD = 0 } access_t;


  std::map<uint32_t, uint64_t> fuse_data;

  std::map<uint32_t, uint64_t> patch_ram;
  
  std::vector<uint32_t> patch_header = {
    0x7b209073,        //0x4214c000 :    	csrw	dscratch0,ra
    0x7b311073,        //0x4214c004 :    	csrw	dscratch1,sp
    0x7c9020f3,        //0x4214c008 :    	csrr	ra,0x7c9
    0xffe0f093,        //0x4214c00c :    	andi	ra,ra,-2
    0x7c909073,        //0x4214c010 :    	csrw	0x7c9,ra
    0x7ca060f3,        //0x4214c014 :    	csrrsi	ra,0x7ca,0
    0x00c0d093,        //0x4214c018 :    	srli	ra,ra,0xc
    0x00c09093,        //0x4214c01c :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c020 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c024 :    	andi	sp,sp,7
    0x00811113,        //0x4214c028 :    	slli	sp,sp,0x8
    0x00110133,        //0x4214c02c :    	add	sp,sp,ra
    0x78010113,        //0x4214c030 :    	addi	sp,sp,1920
    0x78010113,        //0x4214c034 :    	addi	sp,sp,1920
    0x60010113,        //0x4214c038 :    	addi	sp,sp,1536
    0x00313c23,        //0x4214c03c :    	sd	gp,24(sp)
    0x02413023,        //0x4214c040 :    	sd	tp,32(sp)
    0x02513423,        //0x4214c044 :    	sd	t0,40(sp)
    0x02613823,        //0x4214c048 :    	sd	t1,48(sp)
    0x02713c23,        //0x4214c04c :    	sd	t2,56(sp)
    0x04813023,        //0x4214c050 :    	sd	s0,64(sp)
    0x04913423,        //0x4214c054 :    	sd	s1,72(sp)
    0x04a13823,        //0x4214c058 :    	sd	a0,80(sp)
    0x04b13c23,        //0x4214c05c :    	sd	a1,88(sp)
    0x06c13023,        //0x4214c060 :    	sd	a2,96(sp)
    0x06d13423,        //0x4214c064 :    	sd	a3,104(sp)
    0x06e13823,        //0x4214c068 :    	sd	a4,112(sp)
    0x06f13c23,        //0x4214c06c :    	sd	a5,120(sp)
    0x09013023,        //0x4214c070 :    	sd	a6,128(sp)
    0x09113423,        //0x4214c074 :    	sd	a7,136(sp)
    0x09213823,        //0x4214c078 :    	sd	s2,144(sp)
    0x09313c23,        //0x4214c07c :    	sd	s3,152(sp)
    0x0b413023,        //0x4214c080 :    	sd	s4,160(sp)
    0x0b513423,        //0x4214c084 :    	sd	s5,168(sp)
    0x0b613823,        //0x4214c088 :    	sd	s6,176(sp)
    0x0b713c23,        //0x4214c08c :    	sd	s7,184(sp)
    0x0d813023,        //0x4214c090 :    	sd	s8,192(sp)
    0x0d913423,        //0x4214c094 :    	sd	s9,200(sp)
    0x0da13823,        //0x4214c098 :    	sd	s10,208(sp)
    0x0db13c23,        //0x4214c09c :    	sd	s11,216(sp)
    0x0fc13023,        //0x4214c0a0 :    	sd	t3,224(sp)
    0x0fd13423,        //0x4214c0a4 :    	sd	t4,232(sp)
    0x0fe13823,        //0x4214c0a8 :    	sd	t5,240(sp)
    0x0ff13c23,        //0x4214c0ac :    	sd	t6,248(sp)
    0x7b206f73,        //0x4214c0b0 :    	csrrsi	t5,dscratch0,0
    0x7b306ff3,        //0x4214c0b4 :    	csrrsi	t6,dscratch1,0
    0x01e13423,        //0x4214c0b8 :    	sd	t5,8(sp)
    0x01f13823,        //0x4214c0bc :    	sd	t6,16(sp)
    0x7c8061f3,        //0x4214c0c0 :    	csrrsi	gp,0x7c8,0
    0x7b006273,        //0x4214c0c4 :    	csrrsi	tp,dcsr,0
    0x1c027213,        //0x4214c0c8 :    	andi	tp,tp,448
    0x00625213,        //0x4214c0cc :    	srli	tp,tp,0x6
    0x00621213,        //0x4214c0d0 :    	slli	tp,tp,0x6
    0x00408233,        //0x4214c0d4 :    	add	tp,ra,tp
    0x40020213,        //0x4214c0d8 :    	addi	tp,tp,1024 # 400 <tohost-0x6ffffc00>
    0x7b106f73,        //0x4214c0dc :    	csrrsi	t5,dpc,0
    0x00020067,        //0x4214c0e0 :    	jr	tp # 0 <tohost-0x70000000>
    0x7b109073,        //0x4214c0e4 :    	csrw	dpc,ra
    0x7c9020f3,        //0x4214c0e8 :    	csrr	ra,0x7c9
    0xffe0f093,        //0x4214c0ec :    	andi	ra,ra,-2
    0x7c909073,        //0x4214c0f0 :    	csrw	0x7c9,
    0x7ca060f3,        //0x4214c0f4 :    	csrrsi	ra,0x7ca,0
    0x00c0d093,        //0x4214c0f8 :    	srli	ra,ra,0xc
    0x00c09093,        //0x4214c0fc :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c100 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c104 :    	andi	sp,sp,7
    0x00811113,        //0x4214c108 :    	slli	sp,sp,0x8
    0x00110133,        //0x4214c10c :    	add	sp,sp,ra
    0x78010113,        //0x4214c110 :    	addi	sp,sp,1920
    0x78010113,        //0x4214c114 :    	addi	sp,sp,1920
    0x60010113,        //0x4214c118 :    	addi	sp,sp,1536
    0x00813f03,        //0x4214c11c :    	ld	t5,8(sp)
    0x01013f83,        //0x4214c120 :    	ld	t6,16(sp)
    0x7b2f1073,        //0x4214c124 :    	csrw	dscratch0,t5
    0x7b3f9073,        //0x4214c128 :    	csrw	dscratch1,t6
    0x01813183,        //0x4214c12c :    	ld	gp,24(sp)
    0x02013203,        //0x4214c130 :    	ld	tp,32(sp)
    0x02813283,        //0x4214c134 :    	ld	t0,40(sp)
    0x03013303,        //0x4214c138 :    	ld	t1,48(sp)
    0x03813383,        //0x4214c13c :    	ld	t2,56(sp)
    0x04013403,        //0x4214c140 :    	ld	s0,64(sp)
    0x04813483,        //0x4214c144 :    	ld	s1,72(sp)
    0x05013503,        //0x4214c148 :    	ld	a0,80(sp)
    0x05813583,        //0x4214c14c :    	ld	a1,88(sp)
    0x06013603,        //0x4214c150 :    	ld	a2,96(sp)
    0x06813683,        //0x4214c154 :    	ld	a3,104(sp)
    0x07013703,        //0x4214c158 :    	ld	a4,112(sp)
    0x07813783,        //0x4214c15c :    	ld	a5,120(sp)
    0x08013803,        //0x4214c160 :    	ld	a6,128(sp)
    0x08813883,        //0x4214c164 :    	ld	a7,136(sp)
    0x09013903,        //0x4214c168 :    	ld	s2,144(sp)
    0x09813983,        //0x4214c16c :    	ld	s3,152(sp)
    0x0a013a03,        //0x4214c170 :    	ld	s4,160(sp)
    0x0a813a83,        //0x4214c174 :    	ld	s5,168(sp)
    0x0b013b03,        //0x4214c178 :    	ld	s6,176(sp)
    0x0b813b83,        //0x4214c17c :    	ld	s7,184(sp)
    0x0c013c03,        //0x4214c180 :    	ld	s8,192(sp)
    0x0c813c83,        //0x4214c184 :    	ld	s9,200(sp)
    0x0d013d03,        //0x4214c188 :    	ld	s10,208(sp)
    0x0d813d83,        //0x4214c18c :    	ld	s11,216(sp)
    0x0e013e03,        //0x4214c190 :    	ld	t3,224(sp)
    0x0e813e83,        //0x4214c194 :    	ld	t4,232(sp)
    0x0f013f03,        //0x4214c198 :    	ld	t5,240(sp)
    0x0f813f83,        //0x4214c19c :    	ld	t6,248(sp)
    0x7b2060f3,        //0x4214c1a0 :    	csrrsi	ra,dscratch0,0
    0x7b306173,        //0x4214c1a8 :    	csrrsi	sp,dscratch1,0
    0x7b200073,        //0x4214c1b0 :    	dret
  };
  std::vector<uint32_t> patch_trig_0 = {
    0x4214cfb7,        	//lui	t6,0x4214c
    0x500f8f9b,        	//addiw	t6,t6,1280 # 4214c500 <tohost-0x2deb3b00>
    0x000f8067,        	//jr	t6
  };
  std::vector<uint32_t> patch_trig_1 = {
    0x4214dfb7,        	//lui	t6,0x4214d
    0x900f8f9b,        	//addiw	t6,t6,-1792 # 4214c900 <tohost-0x2deb3700>
    0x000f8067,        	//jr	t6
  };
  std::vector<uint32_t> patch_trig_2 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0xd00f8f9b,         //addiw	t6,t6,-768 # 4214cd00 <tohost-0x2deb3300>
    0x000f8067,         //jr	t6
  };
  std::vector<uint32_t> patch_trig_3 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0x100f8f9b,         //addiw	t6,t6,256 # 4214d100 <tohost-0x2deb2f00>
    0x000f8067,         //jr	t6
  };
  std::vector<uint32_t> patch_body_sub = {
    0x00f1d213,          //	srli	tp,gp,0xf
    0x01f27213,          //	andi	tp,tp,31
    0x0141d293,          //	srli	t0,gp,0x14
    0x01f2f293,          //	andi	t0,t0,31
    0x0071d313,          //	srli	t1,gp,0x7
    0x01f37313,          //	andi	t1,t1,31
    0x00321213,          //	slli	tp,tp,0x3
    0x00220233,          //	add	tp,tp,sp
    0x00023203,          //	ld	tp,0(tp) # 0 <tohost-0x70000000>
    0x00329293,          //	slli	t0,t0,0x3
    0x002282b3,          //	add	t0,t0,sp
    0x0002b283,          //	ld	t0,0(t0)
    0xfff2c293,          //	not	t0,t0
    0x00520233,          //	add	tp,tp,t0
    0x00120213,          //	addi	tp,tp,1 # 1 <tohost-0x6fffffff>
    0x00331313,          //	slli	t1,t1,0x3
    0x00230333,          //	add	t1,t1,sp
    0x00433023,          //	sd	tp,0(t1)
    0x7c9023f3,          //	csrr	t2,0x7c9
    0x0023f393,          //	andi	t2,t2,2
    0x00238393,          //	addi	t2,t2,2
    0x007f00b3,          //	add	ra,t5,t2
    0x4214cfb7,          //	lui	t6,0x4214c
    0x0e4f8f9b,          //	addiw	t6,t6,228 # 4214c0e4 <tohost-0x2deb3f1c>
    0x000f8067,          //	jr	t6
  };


  std::vector<uint32_t>  patch_body_blt ={
    0x00f1d213,          // srli	tp,gp,0xf
    0x01f27213,          // andi	tp,tp,31
    0x00321213,          // slli	tp,tp,0x3
    0x00220233,          // add	tp,tp,sp
    0x0141d293,          // srli	t0,gp,0x14
    0x01f2f293,          // andi	t0,t0,31
    0x00329293,          // slli	t0,t0,0x3
    0x002282b3,          // add	t0,t0,sp
    0x00735313,          // srli	t1,t1,0x7
    0x01f37313,          // andi	t1,t1,31
    0x00331313,          // slli	t1,t1,0x3
    0x00230333,          // add	t1,t1,sp
    0x005223b3,          // slt	t2,tp,t0
    0x00400413,          // li	s0,4
    0x007474b3,          // and	s1,s0,t2
    0xfff3c393,          // not	t2,t2
    0x00737533,          // and	a0,t1,t2
    0x00a4e5b3,          // or	a1,s1,a0
    0x00bf00b3,          // add	ra,t5,a1
    0x7c9023f3,          // csrr	t2,0x7c9
    0x0023f393,          // andi	t2,t2,2
    0x00238393,          // addi	t2,t2,2
    0x007f00b3,          // add	ra,t5,t2
    0x4214cfb7,          // lui	t6,0x4214c
    0x0e4f8f9b,          // addiw	t6,t6,228 # 4214c0e4 <preg3+0x14704c>
    0x000f8067           // jr	t6
  };

    std::vector<uint32_t>  patch_body_wfi ={
    0x00000013,         // nop
    0x004f0093,         // addi	ra,t5,4
    0x4214cfb7,         // lui	t6,0x4214c
    0x0e4f8f9b,         // addiw	t6,t6,228 # 4214c0e4 <tohost-0x2deb3f1c>
    0x000f8067,         // jr	t6

  };


  std::vector<uint32_t>  patch_body_any = {
    0x4214dfb7,          //	lui	t6,0x4214d
    0x118f8f9b,          //	addiw	t6,t6,280 # 4214d118 <tohost-0x2deb2ee8>
    0x003fb023,          //	sd	gp,0(t6)
    0x7c902ff3,          //	csrr	t6,0x7c9
    0x001fef93,          //	ori	t6,t6,1
    0x7c9f9073,          //	csrw	0x7c9,t6
    0x00000013,          //	nop
    0x7c902ff3,          //	csrr	t6,0x7c9
    0x002fff93,          //	andi	t6,t6,2
    0x7c9f9073,          //	csrw	0x7c9,t6
    0x7c9023f3,          //	csrr	t2,0x7c9
    0x0023f393,          //	andi	t2,t2,2
    0x00238393,          //	addi	t2,t2,2
    0x007f00b3,          //	add	ra,t5,t2
    0x4214cfb7,          //	lui	t6,0x4214c
    0x0e4f8f9b,          //	addiw	t6,t6,228 # 4214c0e4 <tohost-0x2deb3f1c>
    0x000f8067,          //	jr	t6
  };

  std::vector<uint32_t>  patch_body_amoswap = {
    0x00f1d213,          // srli	tp,gp,0xf
    0x01f27213,          // andi	tp,tp,31
    0x00321213,          // slli	tp,tp,0x3
    0x00220233,          // add	tp,tp,sp
    0x0141d293,          // srli	t0,gp,0x14
    0x01f2f293,          // andi	t0,t0,31
    0x00329293,          // slli	t0,t0,0x3
    0x002282b3,          // add	t0,t0,sp
    0x0071d313,          // srli t1,gp,0x7
    0x01f37313,          // andi	t1,t1,31
    0x00331313,          // slli	t1,t1,0x3
    0x00230333,          // add	t1,t1,sp
    0x00023203,          // ld	tp,0(tp) # 0 <mstatus-0x300>
    0x0002b483,          // ld	s1,0(t0)
    0x7c902573,          // csrr	a0,0x7c9
    0x00156513,          // ori	a0,a0,1
    0x7c951073,          // csrw	0x7c9,a0
    0x100223af,          // lr.w	t2,(tp)
    0x1892242f,          // sc.w	s0,s1,(tp)
    0xfe041ce3,          // bnez	s0,-8
    0x7c902573,          // csrr	a0,0x7c9
    0x00257513,          // andi	a0,a0,2
    0x7c951073,          // csrw	0x7c9,a0
    0x00733023,          // sd	t2,0(t1)
    0x0072b023,          // sd	t2,0(t0)
    0x004f0093,          // addi	ra,t5,4
    0x4214cfb7,          // lui	t6,0x4214c
    0x0e4f8f9b,          // addiw	t6,t6,228 # 4214c0e4 <preg3+0x14704c>
    0x000f8067,          // jr	t6
  };

std::vector<uint32_t>  patch_body_blt_arith = {
    0x00f1d213,          	// srli	tp,gp,0xf
    0x01f27213,          	// andi	tp,tp,31
    0x00321213,          	// slli	tp,tp,0x3
    0x00220233,          	// add	tp,tp,sp
    0x0141d293,          	// srli	t0,gp,0x14
    0x01f2f293,          	// andi	t0,t0,31
    0x00329293,          	// slli	t0,t0,0x3
    0x002282b3,          	// add	t0,t0,sp
    0x0191d313,          	// srli	t1,gp,0x19
    0x03f37493,          	// andi	s1,t1,63
    0x00549493,          	// slli	s1,s1,0x5
    0x04037313,          	// andi	t1,t1,64
    0x00531313,          	// slli	t1,t1,0x5
    0x00936333,          	// or	t1,t1,s1
    0x0071d513,          	// srli	a0,gp,0x7
    0x01e57493,          	// andi	s1,a0,30
    0x00936333,          	// or	t1,t1,s1
    0x00157493,          	// andi	s1,a0,1
    0x00a49493,          	// slli	s1,s1,0xa
    0x00936333,          	// or	t1,t1,s1
    0x03431313,          	// slli	t1,t1,0x34
    0x43435313,          	// srai	t1,t1,0x34
    0x00023203,          	// ld	tp,0(tp) # 0 <mstatus-0x300>
    0x0002b283,          	// ld	t0,0(t0)
    0x40520eb3,          	// sub	t4,tp,t0
    0x43fede13,          	// srai	t3,t4,0x3f
    0x01c373b3,          	// and	t2,t1,t3
    0xfffe4e13,          	// not	t3,t3
    0x7c902473,          	// csrr	s0,0x7c9
    0x00247413,          	// andi	s0,s0,2
    0x00240413,          	// addi	s0,s0,2
    0x01c47433,          	// and	s0,s0,t3
    0x007403b3,          	// add	t2,s0,t2
    0x01e380b3,          	// add	ra,t2,t5
    0x4214cfb7,          	// lui	t6,0x4214c
    0x0e4f8f9b,          	// addiw	t6,t6,228 # 4214c0e4 <preg3+0x14704c>
    0x000f8067          	// jr	t6
  };
}

class pwrmgmt {

  public:

    pwrmgmt(cvm::topology::loc_t, unsigned) {}
    ~pwrmgmt() {}

};
