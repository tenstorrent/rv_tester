// SystemVerilog CSR Defines Package
// Auto-generated from CSR specification

package csr_map_pkg;

// CSR Address Defines
parameter logic [11:0] CYCLE_ADDR = 12'hC00;
parameter logic [11:0] TIME_ADDR = 12'hC01;
parameter logic [11:0] INSTRET_ADDR = 12'hC02;
parameter logic [11:0] HPMCOUNTER3_ADDR = 12'hC03;
parameter logic [11:0] HPMCOUNTER4_ADDR = 12'hC04;
parameter logic [11:0] HPMCOUNTER5_ADDR = 12'hC05;
parameter logic [11:0] HPMCOUNTER6_ADDR = 12'hC06;
parameter logic [11:0] HPMCOUNTER7_ADDR = 12'hC07;
parameter logic [11:0] HPMCOUNTER8_ADDR = 12'hC08;
parameter logic [11:0] HPMCOUNTER9_ADDR = 12'hC09;
parameter logic [11:0] HPMCOUNTER10_ADDR = 12'hC0A;
parameter logic [11:0] HPMCOUNTER11_ADDR = 12'hC0B;
parameter logic [11:0] HPMCOUNTER12_ADDR = 12'hC0C;
parameter logic [11:0] HPMCOUNTER13_ADDR = 12'hC0D;
parameter logic [11:0] HPMCOUNTER14_ADDR = 12'hC0E;
parameter logic [11:0] HPMCOUNTER15_ADDR = 12'hC0F;
parameter logic [11:0] HPMCOUNTER16_ADDR = 12'hC10;
parameter logic [11:0] HPMCOUNTER17_ADDR = 12'hC11;
parameter logic [11:0] HPMCOUNTER18_ADDR = 12'hC12;
parameter logic [11:0] HPMCOUNTER19_ADDR = 12'hC13;
parameter logic [11:0] HPMCOUNTER20_ADDR = 12'hC14;
parameter logic [11:0] HPMCOUNTER21_ADDR = 12'hC15;
parameter logic [11:0] HPMCOUNTER22_ADDR = 12'hC16;
parameter logic [11:0] HPMCOUNTER23_ADDR = 12'hC17;
parameter logic [11:0] HPMCOUNTER24_ADDR = 12'hC18;
parameter logic [11:0] HPMCOUNTER25_ADDR = 12'hC19;
parameter logic [11:0] HPMCOUNTER26_ADDR = 12'hC1A;
parameter logic [11:0] HPMCOUNTER27_ADDR = 12'hC1B;
parameter logic [11:0] HPMCOUNTER28_ADDR = 12'hC1C;
parameter logic [11:0] HPMCOUNTER29_ADDR = 12'hC1D;
parameter logic [11:0] HPMCOUNTER30_ADDR = 12'hC1E;
parameter logic [11:0] HPMCOUNTER31_ADDR = 12'hC1F;
parameter logic [11:0] MISA_ADDR = 12'h301;
parameter logic [11:0] MSTATUS_ADDR = 12'h300;
parameter logic [11:0] MTVEC_ADDR = 12'h305;
parameter logic [11:0] MEDELEG_ADDR = 12'h302;
parameter logic [11:0] MIDELEG_ADDR = 12'h303;
parameter logic [11:0] MIP_ADDR = 12'h344;
parameter logic [11:0] MIE_ADDR = 12'h304;
parameter logic [11:0] MSCRATCH_ADDR = 12'h340;
parameter logic [11:0] MEPC_ADDR = 12'h341;
parameter logic [11:0] MCAUSE_ADDR = 12'h342;
parameter logic [11:0] MTVAL_ADDR = 12'h343;
parameter logic [11:0] MCONFIGPTR_ADDR = 12'hF15;
parameter logic [11:0] MENVCFG_ADDR = 12'h30A;
parameter logic [11:0] MSECCFG_ADDR = 12'h747;
parameter logic [11:0] MCYCLE_ADDR = 12'hB00;
parameter logic [11:0] MINSTRET_ADDR = 12'hB02;
parameter logic [11:0] MHPMCOUNTER3_ADDR = 12'hB03;
parameter logic [11:0] MHPMCOUNTER4_ADDR = 12'hB04;
parameter logic [11:0] MHPMCOUNTER5_ADDR = 12'hB05;
parameter logic [11:0] MHPMCOUNTER6_ADDR = 12'hB06;
parameter logic [11:0] MHPMCOUNTER7_ADDR = 12'hB07;
parameter logic [11:0] MHPMCOUNTER8_ADDR = 12'hB08;
parameter logic [11:0] MHPMCOUNTER9_ADDR = 12'hB09;
parameter logic [11:0] MHPMCOUNTER10_ADDR = 12'hB0A;
parameter logic [11:0] MHPMCOUNTER11_ADDR = 12'hB0B;
parameter logic [11:0] MHPMCOUNTER12_ADDR = 12'hB0C;
parameter logic [11:0] MHPMCOUNTER13_ADDR = 12'hB0D;
parameter logic [11:0] MHPMCOUNTER14_ADDR = 12'hB0E;
parameter logic [11:0] MHPMCOUNTER15_ADDR = 12'hB0F;
parameter logic [11:0] MHPMCOUNTER16_ADDR = 12'hB10;
parameter logic [11:0] MHPMCOUNTER17_ADDR = 12'hB11;
parameter logic [11:0] MHPMCOUNTER18_ADDR = 12'hB12;
parameter logic [11:0] MHPMCOUNTER19_ADDR = 12'hB13;
parameter logic [11:0] MHPMCOUNTER20_ADDR = 12'hB14;
parameter logic [11:0] MHPMCOUNTER21_ADDR = 12'hB15;
parameter logic [11:0] MHPMCOUNTER22_ADDR = 12'hB16;
parameter logic [11:0] MHPMCOUNTER23_ADDR = 12'hB17;
parameter logic [11:0] MHPMCOUNTER24_ADDR = 12'hB18;
parameter logic [11:0] MHPMCOUNTER25_ADDR = 12'hB19;
parameter logic [11:0] MHPMCOUNTER26_ADDR = 12'hB1A;
parameter logic [11:0] MHPMCOUNTER27_ADDR = 12'hB1B;
parameter logic [11:0] MHPMCOUNTER28_ADDR = 12'hB1C;
parameter logic [11:0] MHPMCOUNTER29_ADDR = 12'hB1D;
parameter logic [11:0] MHPMCOUNTER30_ADDR = 12'hB1E;
parameter logic [11:0] MHPMCOUNTER31_ADDR = 12'hB1F;
parameter logic [11:0] MHPMEVENT3_ADDR = 12'h323;
parameter logic [11:0] MHPMEVENT4_ADDR = 12'h324;
parameter logic [11:0] MHPMEVENT5_ADDR = 12'h325;
parameter logic [11:0] MHPMEVENT6_ADDR = 12'h326;
parameter logic [11:0] MHPMEVENT7_ADDR = 12'h327;
parameter logic [11:0] MHPMEVENT8_ADDR = 12'h328;
parameter logic [11:0] MHPMEVENT9_ADDR = 12'h329;
parameter logic [11:0] MHPMEVENT10_ADDR = 12'h32A;
parameter logic [11:0] MHPMEVENT11_ADDR = 12'h32B;
parameter logic [11:0] MHPMEVENT12_ADDR = 12'h32C;
parameter logic [11:0] MHPMEVENT13_ADDR = 12'h32D;
parameter logic [11:0] MHPMEVENT14_ADDR = 12'h32E;
parameter logic [11:0] MHPMEVENT15_ADDR = 12'h32F;
parameter logic [11:0] MHPMEVENT16_ADDR = 12'h330;
parameter logic [11:0] MHPMEVENT17_ADDR = 12'h331;
parameter logic [11:0] MHPMEVENT18_ADDR = 12'h332;
parameter logic [11:0] MHPMEVENT19_ADDR = 12'h333;
parameter logic [11:0] MHPMEVENT20_ADDR = 12'h334;
parameter logic [11:0] MHPMEVENT21_ADDR = 12'h335;
parameter logic [11:0] MHPMEVENT22_ADDR = 12'h336;
parameter logic [11:0] MHPMEVENT23_ADDR = 12'h337;
parameter logic [11:0] MHPMEVENT24_ADDR = 12'h338;
parameter logic [11:0] MHPMEVENT25_ADDR = 12'h339;
parameter logic [11:0] MHPMEVENT26_ADDR = 12'h33A;
parameter logic [11:0] MHPMEVENT27_ADDR = 12'h33B;
parameter logic [11:0] MHPMEVENT28_ADDR = 12'h33C;
parameter logic [11:0] MHPMEVENT29_ADDR = 12'h33D;
parameter logic [11:0] MHPMEVENT30_ADDR = 12'h33E;
parameter logic [11:0] MHPMEVENT31_ADDR = 12'h33F;
parameter logic [11:0] MCOUNTEREN_ADDR = 12'h306;
parameter logic [11:0] MCOUNTINHIBIT_ADDR = 12'h320;
parameter logic [11:0] MISELECT_ADDR = 12'h350;
parameter logic [11:0] MIREG_ADDR = 12'h351;
parameter logic [11:0] MTOPEI_ADDR = 12'h35C;
parameter logic [11:0] MTOPI_ADDR = 12'hFB0;
parameter logic [11:0] MVIEN_ADDR = 12'h308;
parameter logic [11:0] MVIP_ADDR = 12'h309;
parameter logic [11:0] SSTATUS_ADDR = 12'h100;
parameter logic [11:0] STVEC_ADDR = 12'h105;
parameter logic [11:0] SIP_ADDR = 12'h144;
parameter logic [11:0] SIE_ADDR = 12'h104;
parameter logic [11:0] SCOUNTEREN_ADDR = 12'h106;
parameter logic [11:0] SSCRATCH_ADDR = 12'h140;
parameter logic [11:0] SEPC_ADDR = 12'h141;
parameter logic [11:0] SCAUSE_ADDR = 12'h142;
parameter logic [11:0] STVAL_ADDR = 12'h143;
parameter logic [11:0] STIMECMP_ADDR = 12'h14D;
parameter logic [11:0] SENVCFG_ADDR = 12'h10A;
parameter logic [11:0] SATP_ADDR = 12'h180;
parameter logic [11:0] SRMCFG_ADDR = 12'h181;
parameter logic [11:0] SISELECT_ADDR = 12'h150;
parameter logic [11:0] SIREG_ADDR = 12'h151;
parameter logic [11:0] STOPEI_ADDR = 12'h15C;
parameter logic [11:0] STOPI_ADDR = 12'hDB0;
parameter logic [11:0] SEED_ADDR = 12'h015;
parameter logic [11:0] FFLAGS_ADDR = 12'h001;
parameter logic [11:0] FRM_ADDR = 12'h002;
parameter logic [11:0] FCSR_ADDR = 12'h003;
parameter logic [11:0] VSTART_ADDR = 12'h008;
parameter logic [11:0] VXSAT_ADDR = 12'h009;
parameter logic [11:0] VXRM_ADDR = 12'h00A;
parameter logic [11:0] VCSR_ADDR = 12'h00F;
parameter logic [11:0] VL_ADDR = 12'hC20;
parameter logic [11:0] VTYPE_ADDR = 12'hC21;
parameter logic [11:0] VLENB_ADDR = 12'hC22;
parameter logic [11:0] PMPCFG0_ADDR = 12'h3A0;
parameter logic [11:0] PMPCFG2_ADDR = 12'h3A2;
parameter logic [11:0] PMPADDR0_ADDR = 12'h3B0;
parameter logic [11:0] PMPADDR1_ADDR = 12'h3B1;
parameter logic [11:0] PMPADDR2_ADDR = 12'h3B2;
parameter logic [11:0] PMPADDR3_ADDR = 12'h3B3;
parameter logic [11:0] PMPADDR4_ADDR = 12'h3B4;
parameter logic [11:0] PMPADDR5_ADDR = 12'h3B5;
parameter logic [11:0] PMPADDR6_ADDR = 12'h3B6;
parameter logic [11:0] PMPADDR7_ADDR = 12'h3B7;
parameter logic [11:0] PMPADDR8_ADDR = 12'h3B8;
parameter logic [11:0] PMPADDR9_ADDR = 12'h3B9;
parameter logic [11:0] PMPADDR10_ADDR = 12'h3BA;
parameter logic [11:0] PMPADDR11_ADDR = 12'h3BB;
parameter logic [11:0] PMPADDR12_ADDR = 12'h3BC;
parameter logic [11:0] PMPADDR13_ADDR = 12'h3BD;
parameter logic [11:0] PMPADDR14_ADDR = 12'h3BE;
parameter logic [11:0] PMPADDR15_ADDR = 12'h3BF;
parameter logic [11:0] TSELECT_ADDR = 12'h7A0;
parameter logic [11:0] DCSR_ADDR = 12'h7B0;
parameter logic [11:0] DPC_ADDR = 12'h7B1;
parameter logic [11:0] DSCRATCH0_ADDR = 12'h7B2;
parameter logic [11:0] DSCRATCH1_ADDR = 12'h7B3;
parameter logic [11:0] HSTATUS_ADDR = 12'h600;
parameter logic [11:0] HEDELEG_ADDR = 12'h602;
parameter logic [11:0] HIDELEG_ADDR = 12'h603;
parameter logic [11:0] HVIP_ADDR = 12'h645;
parameter logic [11:0] HVIPRIO1_ADDR = 12'h646;
parameter logic [11:0] HVIPRIO2_ADDR = 12'h647;
parameter logic [11:0] HIP_ADDR = 12'h644;
parameter logic [11:0] HIE_ADDR = 12'h604;
parameter logic [11:0] HGEIP_ADDR = 12'hE12;
parameter logic [11:0] HGEIE_ADDR = 12'h607;
parameter logic [11:0] HENVCFG_ADDR = 12'h60A;
parameter logic [11:0] HCOUNTEREN_ADDR = 12'h606;
parameter logic [11:0] HTIMEDELTA_ADDR = 12'h605;
parameter logic [11:0] HTVAL_ADDR = 12'h643;
parameter logic [11:0] HTINST_ADDR = 12'h64A;
parameter logic [11:0] HGATP_ADDR = 12'h680;
parameter logic [11:0] HVIEN_ADDR = 12'h608;
parameter logic [11:0] HVICTL_ADDR = 12'h609;
parameter logic [11:0] VSSTATUS_ADDR = 12'h200;
parameter logic [11:0] VSIP_ADDR = 12'h244;
parameter logic [11:0] VSIE_ADDR = 12'h204;
parameter logic [11:0] VSTVEC_ADDR = 12'h205;
parameter logic [11:0] VSSCRATCH_ADDR = 12'h240;
parameter logic [11:0] VSEPC_ADDR = 12'h241;
parameter logic [11:0] VSCAUSE_ADDR = 12'h242;
parameter logic [11:0] VSTVAL_ADDR = 12'h243;
parameter logic [11:0] VSTIMECMP_ADDR = 12'h24D;
parameter logic [11:0] VSATP_ADDR = 12'h280;
parameter logic [11:0] VSISELECT_ADDR = 12'h250;
parameter logic [11:0] VSIREG_ADDR = 12'h251;
parameter logic [11:0] VSTOPEI_ADDR = 12'h25C;
parameter logic [11:0] VSTOPI_ADDR = 12'hEB0;
parameter logic [11:0] MTINST_ADDR = 12'h34A;
parameter logic [11:0] MTVAL2_ADDR = 12'h34B;
parameter logic [11:0] SCOUNTOVF_ADDR = 12'hDA0;
parameter logic [11:0] MNSTATUS_ADDR = 12'h744;
parameter logic [11:0] MNSCRATCH_ADDR = 12'h740;
parameter logic [11:0] MNEPC_ADDR = 12'h741;
parameter logic [11:0] MNCAUSE_ADDR = 12'h742;
parameter logic [11:0] MSTATEEN0_ADDR = 12'h30C;
parameter logic [11:0] MSTATEEN1_ADDR = 12'h30D;
parameter logic [11:0] MSTATEEN2_ADDR = 12'h30E;
parameter logic [11:0] MSTATEEN3_ADDR = 12'h30F;
parameter logic [11:0] HSTATEEN0_ADDR = 12'h60C;
parameter logic [11:0] HSTATEEN1_ADDR = 12'h60D;
parameter logic [11:0] HSTATEEN2_ADDR = 12'h60E;
parameter logic [11:0] HSTATEEN3_ADDR = 12'h60F;
parameter logic [11:0] SSTATEEN0_ADDR = 12'h10C;
parameter logic [11:0] SSTATEEN1_ADDR = 12'h10D;
parameter logic [11:0] SSTATEEN2_ADDR = 12'h10E;
parameter logic [11:0] SSTATEEN3_ADDR = 12'h10F;

// CSR Size Defines
parameter int CYCLE_SIZE = 64;
parameter int TIME_SIZE = 64;
parameter int INSTRET_SIZE = 64;
parameter int HPMCOUNTER3_SIZE = 64;
parameter int HPMCOUNTER4_SIZE = 64;
parameter int HPMCOUNTER5_SIZE = 64;
parameter int HPMCOUNTER6_SIZE = 64;
parameter int HPMCOUNTER7_SIZE = 64;
parameter int HPMCOUNTER8_SIZE = 64;
parameter int HPMCOUNTER9_SIZE = 64;
parameter int HPMCOUNTER10_SIZE = 64;
parameter int HPMCOUNTER11_SIZE = 64;
parameter int HPMCOUNTER12_SIZE = 64;
parameter int HPMCOUNTER13_SIZE = 64;
parameter int HPMCOUNTER14_SIZE = 64;
parameter int HPMCOUNTER15_SIZE = 64;
parameter int HPMCOUNTER16_SIZE = 64;
parameter int HPMCOUNTER17_SIZE = 64;
parameter int HPMCOUNTER18_SIZE = 64;
parameter int HPMCOUNTER19_SIZE = 64;
parameter int HPMCOUNTER20_SIZE = 64;
parameter int HPMCOUNTER21_SIZE = 64;
parameter int HPMCOUNTER22_SIZE = 64;
parameter int HPMCOUNTER23_SIZE = 64;
parameter int HPMCOUNTER24_SIZE = 64;
parameter int HPMCOUNTER25_SIZE = 64;
parameter int HPMCOUNTER26_SIZE = 64;
parameter int HPMCOUNTER27_SIZE = 64;
parameter int HPMCOUNTER28_SIZE = 64;
parameter int HPMCOUNTER29_SIZE = 64;
parameter int HPMCOUNTER30_SIZE = 64;
parameter int HPMCOUNTER31_SIZE = 64;
parameter int MISA_SIZE = 64;
parameter int MSTATUS_SIZE = 64;
parameter int MTVEC_SIZE = 64;
parameter int MEDELEG_SIZE = 64;
parameter int MIDELEG_SIZE = 64;
parameter int MIP_SIZE = 64;
parameter int MIE_SIZE = 64;
parameter int MSCRATCH_SIZE = 64;
parameter int MEPC_SIZE = 64;
parameter int MCAUSE_SIZE = 64;
parameter int MTVAL_SIZE = 64;
parameter int MCONFIGPTR_SIZE = 64;
parameter int MENVCFG_SIZE = 64;
parameter int MSECCFG_SIZE = 64;
parameter int MCYCLE_SIZE = 64;
parameter int MINSTRET_SIZE = 64;
parameter int MHPMCOUNTER3_SIZE = 64;
parameter int MHPMCOUNTER4_SIZE = 64;
parameter int MHPMCOUNTER5_SIZE = 64;
parameter int MHPMCOUNTER6_SIZE = 64;
parameter int MHPMCOUNTER7_SIZE = 64;
parameter int MHPMCOUNTER8_SIZE = 64;
parameter int MHPMCOUNTER9_SIZE = 64;
parameter int MHPMCOUNTER10_SIZE = 64;
parameter int MHPMCOUNTER11_SIZE = 64;
parameter int MHPMCOUNTER12_SIZE = 64;
parameter int MHPMCOUNTER13_SIZE = 64;
parameter int MHPMCOUNTER14_SIZE = 64;
parameter int MHPMCOUNTER15_SIZE = 64;
parameter int MHPMCOUNTER16_SIZE = 64;
parameter int MHPMCOUNTER17_SIZE = 64;
parameter int MHPMCOUNTER18_SIZE = 64;
parameter int MHPMCOUNTER19_SIZE = 64;
parameter int MHPMCOUNTER20_SIZE = 64;
parameter int MHPMCOUNTER21_SIZE = 64;
parameter int MHPMCOUNTER22_SIZE = 64;
parameter int MHPMCOUNTER23_SIZE = 64;
parameter int MHPMCOUNTER24_SIZE = 64;
parameter int MHPMCOUNTER25_SIZE = 64;
parameter int MHPMCOUNTER26_SIZE = 64;
parameter int MHPMCOUNTER27_SIZE = 64;
parameter int MHPMCOUNTER28_SIZE = 64;
parameter int MHPMCOUNTER29_SIZE = 64;
parameter int MHPMCOUNTER30_SIZE = 64;
parameter int MHPMCOUNTER31_SIZE = 64;
parameter int MHPMEVENT3_SIZE = 64;
parameter int MHPMEVENT4_SIZE = 64;
parameter int MHPMEVENT5_SIZE = 64;
parameter int MHPMEVENT6_SIZE = 64;
parameter int MHPMEVENT7_SIZE = 64;
parameter int MHPMEVENT8_SIZE = 64;
parameter int MHPMEVENT9_SIZE = 64;
parameter int MHPMEVENT10_SIZE = 64;
parameter int MHPMEVENT11_SIZE = 64;
parameter int MHPMEVENT12_SIZE = 64;
parameter int MHPMEVENT13_SIZE = 64;
parameter int MHPMEVENT14_SIZE = 64;
parameter int MHPMEVENT15_SIZE = 64;
parameter int MHPMEVENT16_SIZE = 64;
parameter int MHPMEVENT17_SIZE = 64;
parameter int MHPMEVENT18_SIZE = 64;
parameter int MHPMEVENT19_SIZE = 64;
parameter int MHPMEVENT20_SIZE = 64;
parameter int MHPMEVENT21_SIZE = 64;
parameter int MHPMEVENT22_SIZE = 64;
parameter int MHPMEVENT23_SIZE = 64;
parameter int MHPMEVENT24_SIZE = 64;
parameter int MHPMEVENT25_SIZE = 64;
parameter int MHPMEVENT26_SIZE = 64;
parameter int MHPMEVENT27_SIZE = 64;
parameter int MHPMEVENT28_SIZE = 64;
parameter int MHPMEVENT29_SIZE = 64;
parameter int MHPMEVENT30_SIZE = 64;
parameter int MHPMEVENT31_SIZE = 64;
parameter int MCOUNTEREN_SIZE = 32;
parameter int MCOUNTINHIBIT_SIZE = 32;
parameter int MISELECT_SIZE = 64;
parameter int MIREG_SIZE = 64;
parameter int MTOPEI_SIZE = 64;
parameter int MTOPI_SIZE = 64;
parameter int MVIEN_SIZE = 64;
parameter int MVIP_SIZE = 64;
parameter int SSTATUS_SIZE = 64;
parameter int STVEC_SIZE = 64;
parameter int SIP_SIZE = 64;
parameter int SIE_SIZE = 64;
parameter int SCOUNTEREN_SIZE = 32;
parameter int SSCRATCH_SIZE = 64;
parameter int SEPC_SIZE = 64;
parameter int SCAUSE_SIZE = 64;
parameter int STVAL_SIZE = 64;
parameter int STIMECMP_SIZE = 64;
parameter int SENVCFG_SIZE = 64;
parameter int SATP_SIZE = 64;
parameter int SRMCFG_SIZE = 64;
parameter int SISELECT_SIZE = 64;
parameter int SIREG_SIZE = 64;
parameter int STOPEI_SIZE = 64;
parameter int STOPI_SIZE = 64;
parameter int SEED_SIZE = 32;
parameter int FFLAGS_SIZE = 5;
parameter int FRM_SIZE = 3;
parameter int FCSR_SIZE = 32;
parameter int VSTART_SIZE = 64;
parameter int VXSAT_SIZE = 1;
parameter int VXRM_SIZE = 2;
parameter int VCSR_SIZE = 3;
parameter int VL_SIZE = 64;
parameter int VTYPE_SIZE = 64;
parameter int VLENB_SIZE = 64;
parameter int PMPCFG0_SIZE = 64;
parameter int PMPCFG2_SIZE = 64;
parameter int PMPADDR0_SIZE = 64;
parameter int PMPADDR1_SIZE = 64;
parameter int PMPADDR2_SIZE = 64;
parameter int PMPADDR3_SIZE = 64;
parameter int PMPADDR4_SIZE = 64;
parameter int PMPADDR5_SIZE = 64;
parameter int PMPADDR6_SIZE = 64;
parameter int PMPADDR7_SIZE = 64;
parameter int PMPADDR8_SIZE = 64;
parameter int PMPADDR9_SIZE = 64;
parameter int PMPADDR10_SIZE = 64;
parameter int PMPADDR11_SIZE = 64;
parameter int PMPADDR12_SIZE = 64;
parameter int PMPADDR13_SIZE = 64;
parameter int PMPADDR14_SIZE = 64;
parameter int PMPADDR15_SIZE = 64;
parameter int TSELECT_SIZE = 64;
parameter int DCSR_SIZE = 64;
parameter int DPC_SIZE = 64;
parameter int DSCRATCH0_SIZE = 64;
parameter int DSCRATCH1_SIZE = 64;
parameter int HSTATUS_SIZE = 64;
parameter int HEDELEG_SIZE = 64;
parameter int HIDELEG_SIZE = 64;
parameter int HVIP_SIZE = 64;
parameter int HVIPRIO1_SIZE = 64;
parameter int HVIPRIO2_SIZE = 64;
parameter int HIP_SIZE = 64;
parameter int HIE_SIZE = 64;
parameter int HGEIP_SIZE = 64;
parameter int HGEIE_SIZE = 64;
parameter int HENVCFG_SIZE = 64;
parameter int HCOUNTEREN_SIZE = 32;
parameter int HTIMEDELTA_SIZE = 64;
parameter int HTVAL_SIZE = 64;
parameter int HTINST_SIZE = 64;
parameter int HGATP_SIZE = 64;
parameter int HVIEN_SIZE = 64;
parameter int HVICTL_SIZE = 64;
parameter int VSSTATUS_SIZE = 64;
parameter int VSIP_SIZE = 16;
parameter int VSIE_SIZE = 16;
parameter int VSTVEC_SIZE = 64;
parameter int VSSCRATCH_SIZE = 64;
parameter int VSEPC_SIZE = 64;
parameter int VSCAUSE_SIZE = 64;
parameter int VSTVAL_SIZE = 64;
parameter int VSTIMECMP_SIZE = 64;
parameter int VSATP_SIZE = 64;
parameter int VSISELECT_SIZE = 64;
parameter int VSIREG_SIZE = 64;
parameter int VSTOPEI_SIZE = 64;
parameter int VSTOPI_SIZE = 64;
parameter int MTINST_SIZE = 64;
parameter int MTVAL2_SIZE = 64;
parameter int SCOUNTOVF_SIZE = 64;
parameter int MNSTATUS_SIZE = 64;
parameter int MNSCRATCH_SIZE = 64;
parameter int MNEPC_SIZE = 64;
parameter int MNCAUSE_SIZE = 64;
parameter int MSTATEEN0_SIZE = 64;
parameter int MSTATEEN1_SIZE = 64;
parameter int MSTATEEN2_SIZE = 64;
parameter int MSTATEEN3_SIZE = 64;
parameter int HSTATEEN0_SIZE = 64;
parameter int HSTATEEN1_SIZE = 64;
parameter int HSTATEEN2_SIZE = 64;
parameter int HSTATEEN3_SIZE = 64;
parameter int SSTATEEN0_SIZE = 32;
parameter int SSTATEEN1_SIZE = 32;
parameter int SSTATEEN2_SIZE = 32;
parameter int SSTATEEN3_SIZE = 32;

// CYCLE CSR Field Defines
parameter int CYCLE_CYCLE_MSB = 63;
parameter int CYCLE_CYCLE_LSB = 0;
parameter int CYCLE_CYCLE_WIDTH = 64;
parameter logic [63:0] CYCLE_CYCLE_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] CYCLE_CYCLE_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string CYCLE_CYCLE_SW_TYPE = "WARL";


// TIME CSR Field Defines
parameter int TIME_TIME_MSB = 63;
parameter int TIME_TIME_LSB = 0;
parameter int TIME_TIME_WIDTH = 64;
parameter logic [63:0] TIME_TIME_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] TIME_TIME_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string TIME_TIME_SW_TYPE = "WARL";


// INSTRET CSR Field Defines
parameter int INSTRET_INSTRET_MSB = 63;
parameter int INSTRET_INSTRET_LSB = 0;
parameter int INSTRET_INSTRET_WIDTH = 64;
parameter logic [63:0] INSTRET_INSTRET_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] INSTRET_INSTRET_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string INSTRET_INSTRET_SW_TYPE = "WARL";


// HPMCOUNTER3 CSR Field Defines
parameter int HPMCOUNTER3_HPMCOUNTER3_MSB = 63;
parameter int HPMCOUNTER3_HPMCOUNTER3_LSB = 0;
parameter int HPMCOUNTER3_HPMCOUNTER3_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER3_HPMCOUNTER3_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER3_HPMCOUNTER3_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER3_HPMCOUNTER3_SW_TYPE = "WARL";


// HPMCOUNTER4 CSR Field Defines
parameter int HPMCOUNTER4_HPMCOUNTER4_MSB = 63;
parameter int HPMCOUNTER4_HPMCOUNTER4_LSB = 0;
parameter int HPMCOUNTER4_HPMCOUNTER4_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER4_HPMCOUNTER4_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER4_HPMCOUNTER4_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER4_HPMCOUNTER4_SW_TYPE = "WARL";


// HPMCOUNTER5 CSR Field Defines
parameter int HPMCOUNTER5_HPMCOUNTER5_MSB = 63;
parameter int HPMCOUNTER5_HPMCOUNTER5_LSB = 0;
parameter int HPMCOUNTER5_HPMCOUNTER5_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER5_HPMCOUNTER5_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER5_HPMCOUNTER5_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER5_HPMCOUNTER5_SW_TYPE = "WARL";


// HPMCOUNTER6 CSR Field Defines
parameter int HPMCOUNTER6_HPMCOUNTER6_MSB = 63;
parameter int HPMCOUNTER6_HPMCOUNTER6_LSB = 0;
parameter int HPMCOUNTER6_HPMCOUNTER6_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER6_HPMCOUNTER6_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER6_HPMCOUNTER6_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER6_HPMCOUNTER6_SW_TYPE = "WARL";


// HPMCOUNTER7 CSR Field Defines
parameter int HPMCOUNTER7_HPMCOUNTER7_MSB = 63;
parameter int HPMCOUNTER7_HPMCOUNTER7_LSB = 0;
parameter int HPMCOUNTER7_HPMCOUNTER7_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER7_HPMCOUNTER7_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER7_HPMCOUNTER7_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER7_HPMCOUNTER7_SW_TYPE = "WARL";


// HPMCOUNTER8 CSR Field Defines
parameter int HPMCOUNTER8_HPMCOUNTER8_MSB = 63;
parameter int HPMCOUNTER8_HPMCOUNTER8_LSB = 0;
parameter int HPMCOUNTER8_HPMCOUNTER8_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER8_HPMCOUNTER8_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER8_HPMCOUNTER8_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER8_HPMCOUNTER8_SW_TYPE = "WARL";


// HPMCOUNTER9 CSR Field Defines
parameter int HPMCOUNTER9_HPMCOUNTER9_MSB = 63;
parameter int HPMCOUNTER9_HPMCOUNTER9_LSB = 0;
parameter int HPMCOUNTER9_HPMCOUNTER9_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER9_HPMCOUNTER9_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER9_HPMCOUNTER9_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER9_HPMCOUNTER9_SW_TYPE = "WARL";


// HPMCOUNTER10 CSR Field Defines
parameter int HPMCOUNTER10_HPMCOUNTER10_MSB = 63;
parameter int HPMCOUNTER10_HPMCOUNTER10_LSB = 0;
parameter int HPMCOUNTER10_HPMCOUNTER10_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER10_HPMCOUNTER10_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER10_HPMCOUNTER10_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER10_HPMCOUNTER10_SW_TYPE = "WARL";


// HPMCOUNTER11 CSR Field Defines
parameter int HPMCOUNTER11_HPMCOUNTER11_MSB = 63;
parameter int HPMCOUNTER11_HPMCOUNTER11_LSB = 0;
parameter int HPMCOUNTER11_HPMCOUNTER11_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER11_HPMCOUNTER11_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER11_HPMCOUNTER11_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER11_HPMCOUNTER11_SW_TYPE = "WARL";


// HPMCOUNTER12 CSR Field Defines
parameter int HPMCOUNTER12_HPMCOUNTER12_MSB = 63;
parameter int HPMCOUNTER12_HPMCOUNTER12_LSB = 0;
parameter int HPMCOUNTER12_HPMCOUNTER12_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER12_HPMCOUNTER12_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER12_HPMCOUNTER12_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER12_HPMCOUNTER12_SW_TYPE = "WARL";


// HPMCOUNTER13 CSR Field Defines
parameter int HPMCOUNTER13_HPMCOUNTER13_MSB = 63;
parameter int HPMCOUNTER13_HPMCOUNTER13_LSB = 0;
parameter int HPMCOUNTER13_HPMCOUNTER13_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER13_HPMCOUNTER13_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER13_HPMCOUNTER13_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER13_HPMCOUNTER13_SW_TYPE = "WARL";


// HPMCOUNTER14 CSR Field Defines
parameter int HPMCOUNTER14_HPMCOUNTER14_MSB = 63;
parameter int HPMCOUNTER14_HPMCOUNTER14_LSB = 0;
parameter int HPMCOUNTER14_HPMCOUNTER14_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER14_HPMCOUNTER14_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER14_HPMCOUNTER14_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER14_HPMCOUNTER14_SW_TYPE = "WARL";


// HPMCOUNTER15 CSR Field Defines
parameter int HPMCOUNTER15_HPMCOUNTER15_MSB = 63;
parameter int HPMCOUNTER15_HPMCOUNTER15_LSB = 0;
parameter int HPMCOUNTER15_HPMCOUNTER15_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER15_HPMCOUNTER15_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER15_HPMCOUNTER15_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER15_HPMCOUNTER15_SW_TYPE = "WARL";


// HPMCOUNTER16 CSR Field Defines
parameter int HPMCOUNTER16_HPMCOUNTER16_MSB = 63;
parameter int HPMCOUNTER16_HPMCOUNTER16_LSB = 0;
parameter int HPMCOUNTER16_HPMCOUNTER16_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER16_HPMCOUNTER16_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER16_HPMCOUNTER16_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER16_HPMCOUNTER16_SW_TYPE = "WARL";


// HPMCOUNTER17 CSR Field Defines
parameter int HPMCOUNTER17_HPMCOUNTER17_MSB = 63;
parameter int HPMCOUNTER17_HPMCOUNTER17_LSB = 0;
parameter int HPMCOUNTER17_HPMCOUNTER17_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER17_HPMCOUNTER17_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER17_HPMCOUNTER17_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER17_HPMCOUNTER17_SW_TYPE = "WARL";


// HPMCOUNTER18 CSR Field Defines
parameter int HPMCOUNTER18_HPMCOUNTER18_MSB = 63;
parameter int HPMCOUNTER18_HPMCOUNTER18_LSB = 0;
parameter int HPMCOUNTER18_HPMCOUNTER18_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER18_HPMCOUNTER18_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER18_HPMCOUNTER18_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER18_HPMCOUNTER18_SW_TYPE = "WARL";


// HPMCOUNTER19 CSR Field Defines
parameter int HPMCOUNTER19_HPMCOUNTER19_MSB = 63;
parameter int HPMCOUNTER19_HPMCOUNTER19_LSB = 0;
parameter int HPMCOUNTER19_HPMCOUNTER19_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER19_HPMCOUNTER19_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER19_HPMCOUNTER19_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER19_HPMCOUNTER19_SW_TYPE = "WARL";


// HPMCOUNTER20 CSR Field Defines
parameter int HPMCOUNTER20_HPMCOUNTER20_MSB = 63;
parameter int HPMCOUNTER20_HPMCOUNTER20_LSB = 0;
parameter int HPMCOUNTER20_HPMCOUNTER20_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER20_HPMCOUNTER20_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER20_HPMCOUNTER20_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER20_HPMCOUNTER20_SW_TYPE = "WARL";


// HPMCOUNTER21 CSR Field Defines
parameter int HPMCOUNTER21_HPMCOUNTER21_MSB = 63;
parameter int HPMCOUNTER21_HPMCOUNTER21_LSB = 0;
parameter int HPMCOUNTER21_HPMCOUNTER21_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER21_HPMCOUNTER21_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER21_HPMCOUNTER21_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER21_HPMCOUNTER21_SW_TYPE = "WARL";


// HPMCOUNTER22 CSR Field Defines
parameter int HPMCOUNTER22_HPMCOUNTER22_MSB = 63;
parameter int HPMCOUNTER22_HPMCOUNTER22_LSB = 0;
parameter int HPMCOUNTER22_HPMCOUNTER22_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER22_HPMCOUNTER22_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER22_HPMCOUNTER22_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER22_HPMCOUNTER22_SW_TYPE = "WARL";


// HPMCOUNTER23 CSR Field Defines
parameter int HPMCOUNTER23_HPMCOUNTER23_MSB = 63;
parameter int HPMCOUNTER23_HPMCOUNTER23_LSB = 0;
parameter int HPMCOUNTER23_HPMCOUNTER23_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER23_HPMCOUNTER23_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER23_HPMCOUNTER23_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER23_HPMCOUNTER23_SW_TYPE = "WARL";


// HPMCOUNTER24 CSR Field Defines
parameter int HPMCOUNTER24_HPMCOUNTER24_MSB = 63;
parameter int HPMCOUNTER24_HPMCOUNTER24_LSB = 0;
parameter int HPMCOUNTER24_HPMCOUNTER24_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER24_HPMCOUNTER24_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER24_HPMCOUNTER24_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER24_HPMCOUNTER24_SW_TYPE = "WARL";


// HPMCOUNTER25 CSR Field Defines
parameter int HPMCOUNTER25_HPMCOUNTER25_MSB = 63;
parameter int HPMCOUNTER25_HPMCOUNTER25_LSB = 0;
parameter int HPMCOUNTER25_HPMCOUNTER25_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER25_HPMCOUNTER25_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER25_HPMCOUNTER25_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER25_HPMCOUNTER25_SW_TYPE = "WARL";


// HPMCOUNTER26 CSR Field Defines
parameter int HPMCOUNTER26_HPMCOUNTER26_MSB = 63;
parameter int HPMCOUNTER26_HPMCOUNTER26_LSB = 0;
parameter int HPMCOUNTER26_HPMCOUNTER26_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER26_HPMCOUNTER26_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER26_HPMCOUNTER26_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER26_HPMCOUNTER26_SW_TYPE = "WARL";


// HPMCOUNTER27 CSR Field Defines
parameter int HPMCOUNTER27_HPMCOUNTER27_MSB = 63;
parameter int HPMCOUNTER27_HPMCOUNTER27_LSB = 0;
parameter int HPMCOUNTER27_HPMCOUNTER27_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER27_HPMCOUNTER27_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER27_HPMCOUNTER27_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER27_HPMCOUNTER27_SW_TYPE = "WARL";


// HPMCOUNTER28 CSR Field Defines
parameter int HPMCOUNTER28_HPMCOUNTER28_MSB = 63;
parameter int HPMCOUNTER28_HPMCOUNTER28_LSB = 0;
parameter int HPMCOUNTER28_HPMCOUNTER28_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER28_HPMCOUNTER28_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER28_HPMCOUNTER28_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER28_HPMCOUNTER28_SW_TYPE = "WARL";


// HPMCOUNTER29 CSR Field Defines
parameter int HPMCOUNTER29_HPMCOUNTER29_MSB = 63;
parameter int HPMCOUNTER29_HPMCOUNTER29_LSB = 0;
parameter int HPMCOUNTER29_HPMCOUNTER29_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER29_HPMCOUNTER29_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER29_HPMCOUNTER29_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER29_HPMCOUNTER29_SW_TYPE = "WARL";


// HPMCOUNTER30 CSR Field Defines
parameter int HPMCOUNTER30_HPMCOUNTER30_MSB = 63;
parameter int HPMCOUNTER30_HPMCOUNTER30_LSB = 0;
parameter int HPMCOUNTER30_HPMCOUNTER30_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER30_HPMCOUNTER30_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER30_HPMCOUNTER30_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER30_HPMCOUNTER30_SW_TYPE = "WARL";


// HPMCOUNTER31 CSR Field Defines
parameter int HPMCOUNTER31_HPMCOUNTER31_MSB = 63;
parameter int HPMCOUNTER31_HPMCOUNTER31_LSB = 0;
parameter int HPMCOUNTER31_HPMCOUNTER31_WIDTH = 64;
parameter logic [63:0] HPMCOUNTER31_HPMCOUNTER31_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HPMCOUNTER31_HPMCOUNTER31_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HPMCOUNTER31_HPMCOUNTER31_SW_TYPE = "WARL";


// MISA CSR Field Defines
parameter int MISA_MXL_MSB = 63;
parameter int MISA_MXL_LSB = 62;
parameter int MISA_MXL_WIDTH = 2;
parameter logic [1:0] MISA_MXL_RESET = 64'h0000000000000002[63:62];
parameter logic [63:0] MISA_MXL_MASK = 64'hC000000000000000;
parameter string MISA_MXL_SW_TYPE = "WARL";

parameter int MISA_WLRL0_MSB = 61;
parameter int MISA_WLRL0_LSB = 26;
parameter int MISA_WLRL0_WIDTH = 36;
parameter logic [35:0] MISA_WLRL0_RESET = 64'h0000000000000000[61:26];
parameter logic [63:0] MISA_WLRL0_MASK = 64'h3FFFFFFFFC000000;
parameter string MISA_WLRL0_SW_TYPE = "WARL";

parameter int MISA_Z_MSB = 25;
parameter int MISA_Z_LSB = 25;
parameter int MISA_Z_WIDTH = 1;
parameter logic [0:0] MISA_Z_RESET = 64'h0000000000000000[25:25];
parameter logic [63:0] MISA_Z_MASK = 64'h0000000002000000;
parameter string MISA_Z_SW_TYPE = "WARL";

parameter int MISA_Y_MSB = 24;
parameter int MISA_Y_LSB = 24;
parameter int MISA_Y_WIDTH = 1;
parameter logic [0:0] MISA_Y_RESET = 64'h0000000000000000[24:24];
parameter logic [63:0] MISA_Y_MASK = 64'h0000000001000000;
parameter string MISA_Y_SW_TYPE = "WARL";

parameter int MISA_X_MSB = 23;
parameter int MISA_X_LSB = 23;
parameter int MISA_X_WIDTH = 1;
parameter logic [0:0] MISA_X_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] MISA_X_MASK = 64'h0000000000800000;
parameter string MISA_X_SW_TYPE = "WARL";

parameter int MISA_W_MSB = 22;
parameter int MISA_W_LSB = 22;
parameter int MISA_W_WIDTH = 1;
parameter logic [0:0] MISA_W_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] MISA_W_MASK = 64'h0000000000400000;
parameter string MISA_W_SW_TYPE = "WARL";

parameter int MISA_V_MSB = 21;
parameter int MISA_V_LSB = 21;
parameter int MISA_V_WIDTH = 1;
parameter logic [0:0] MISA_V_RESET = 64'h0000000000000001[21:21];
parameter logic [63:0] MISA_V_MASK = 64'h0000000000200000;
parameter string MISA_V_SW_TYPE = "WARL";

parameter int MISA_U_MSB = 20;
parameter int MISA_U_LSB = 20;
parameter int MISA_U_WIDTH = 1;
parameter logic [0:0] MISA_U_RESET = 64'h0000000000000001[20:20];
parameter logic [63:0] MISA_U_MASK = 64'h0000000000100000;
parameter string MISA_U_SW_TYPE = "WARL";

parameter int MISA_T_MSB = 19;
parameter int MISA_T_LSB = 19;
parameter int MISA_T_WIDTH = 1;
parameter logic [0:0] MISA_T_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] MISA_T_MASK = 64'h0000000000080000;
parameter string MISA_T_SW_TYPE = "WARL";

parameter int MISA_S_MSB = 18;
parameter int MISA_S_LSB = 18;
parameter int MISA_S_WIDTH = 1;
parameter logic [0:0] MISA_S_RESET = 64'h0000000000000001[18:18];
parameter logic [63:0] MISA_S_MASK = 64'h0000000000040000;
parameter string MISA_S_SW_TYPE = "WARL";

parameter int MISA_R_MSB = 17;
parameter int MISA_R_LSB = 17;
parameter int MISA_R_WIDTH = 1;
parameter logic [0:0] MISA_R_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] MISA_R_MASK = 64'h0000000000020000;
parameter string MISA_R_SW_TYPE = "WARL";

parameter int MISA_Q_MSB = 16;
parameter int MISA_Q_LSB = 16;
parameter int MISA_Q_WIDTH = 1;
parameter logic [0:0] MISA_Q_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] MISA_Q_MASK = 64'h0000000000010000;
parameter string MISA_Q_SW_TYPE = "WARL";

parameter int MISA_P_MSB = 15;
parameter int MISA_P_LSB = 15;
parameter int MISA_P_WIDTH = 1;
parameter logic [0:0] MISA_P_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] MISA_P_MASK = 64'h0000000000008000;
parameter string MISA_P_SW_TYPE = "WARL";

parameter int MISA_O_MSB = 14;
parameter int MISA_O_LSB = 14;
parameter int MISA_O_WIDTH = 1;
parameter logic [0:0] MISA_O_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] MISA_O_MASK = 64'h0000000000004000;
parameter string MISA_O_SW_TYPE = "WARL";

parameter int MISA_N_MSB = 13;
parameter int MISA_N_LSB = 13;
parameter int MISA_N_WIDTH = 1;
parameter logic [0:0] MISA_N_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MISA_N_MASK = 64'h0000000000002000;
parameter string MISA_N_SW_TYPE = "WARL";

parameter int MISA_M_MSB = 12;
parameter int MISA_M_LSB = 12;
parameter int MISA_M_WIDTH = 1;
parameter logic [0:0] MISA_M_RESET = 64'h0000000000000001[12:12];
parameter logic [63:0] MISA_M_MASK = 64'h0000000000001000;
parameter string MISA_M_SW_TYPE = "WARL";

parameter int MISA_L_MSB = 11;
parameter int MISA_L_LSB = 11;
parameter int MISA_L_WIDTH = 1;
parameter logic [0:0] MISA_L_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MISA_L_MASK = 64'h0000000000000800;
parameter string MISA_L_SW_TYPE = "WARL";

parameter int MISA_K_MSB = 10;
parameter int MISA_K_LSB = 10;
parameter int MISA_K_WIDTH = 1;
parameter logic [0:0] MISA_K_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MISA_K_MASK = 64'h0000000000000400;
parameter string MISA_K_SW_TYPE = "WARL";

parameter int MISA_J_MSB = 9;
parameter int MISA_J_LSB = 9;
parameter int MISA_J_WIDTH = 1;
parameter logic [0:0] MISA_J_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MISA_J_MASK = 64'h0000000000000200;
parameter string MISA_J_SW_TYPE = "WARL";

parameter int MISA_I_MSB = 8;
parameter int MISA_I_LSB = 8;
parameter int MISA_I_WIDTH = 1;
parameter logic [0:0] MISA_I_RESET = 64'h0000000000000001[8:8];
parameter logic [63:0] MISA_I_MASK = 64'h0000000000000100;
parameter string MISA_I_SW_TYPE = "WARL";

parameter int MISA_H_MSB = 7;
parameter int MISA_H_LSB = 7;
parameter int MISA_H_WIDTH = 1;
parameter logic [0:0] MISA_H_RESET = 64'h0000000000000001[7:7];
parameter logic [63:0] MISA_H_MASK = 64'h0000000000000080;
parameter string MISA_H_SW_TYPE = "WARL";

parameter int MISA_G_MSB = 6;
parameter int MISA_G_LSB = 6;
parameter int MISA_G_WIDTH = 1;
parameter logic [0:0] MISA_G_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MISA_G_MASK = 64'h0000000000000040;
parameter string MISA_G_SW_TYPE = "WARL";

parameter int MISA_F_MSB = 5;
parameter int MISA_F_LSB = 5;
parameter int MISA_F_WIDTH = 1;
parameter logic [0:0] MISA_F_RESET = 64'h0000000000000001[5:5];
parameter logic [63:0] MISA_F_MASK = 64'h0000000000000020;
parameter string MISA_F_SW_TYPE = "WARL";

parameter int MISA_E_MSB = 4;
parameter int MISA_E_LSB = 4;
parameter int MISA_E_WIDTH = 1;
parameter logic [0:0] MISA_E_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] MISA_E_MASK = 64'h0000000000000010;
parameter string MISA_E_SW_TYPE = "WARL";

parameter int MISA_D_MSB = 3;
parameter int MISA_D_LSB = 3;
parameter int MISA_D_WIDTH = 1;
parameter logic [0:0] MISA_D_RESET = 64'h0000000000000001[3:3];
parameter logic [63:0] MISA_D_MASK = 64'h0000000000000008;
parameter string MISA_D_SW_TYPE = "WARL";

parameter int MISA_C_MSB = 2;
parameter int MISA_C_LSB = 2;
parameter int MISA_C_WIDTH = 1;
parameter logic [0:0] MISA_C_RESET = 64'h0000000000000001[2:2];
parameter logic [63:0] MISA_C_MASK = 64'h0000000000000004;
parameter string MISA_C_SW_TYPE = "WARL";

parameter int MISA_B_MSB = 1;
parameter int MISA_B_LSB = 1;
parameter int MISA_B_WIDTH = 1;
parameter logic [0:0] MISA_B_RESET = 64'h0000000000000001[1:1];
parameter logic [63:0] MISA_B_MASK = 64'h0000000000000002;
parameter string MISA_B_SW_TYPE = "Read-Only";

parameter int MISA_A_MSB = 0;
parameter int MISA_A_LSB = 0;
parameter int MISA_A_WIDTH = 1;
parameter logic [0:0] MISA_A_RESET = 64'h0000000000000001[0:0];
parameter logic [63:0] MISA_A_MASK = 64'h0000000000000001;
parameter string MISA_A_SW_TYPE = "WARL";


// MSTATUS CSR Field Defines
parameter int MSTATUS_SD_MSB = 63;
parameter int MSTATUS_SD_LSB = 63;
parameter int MSTATUS_SD_WIDTH = 1;
parameter logic [0:0] MSTATUS_SD_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MSTATUS_SD_MASK = 64'h8000000000000000;
parameter string MSTATUS_SD_SW_TYPE = "WARL";

parameter int MSTATUS_MSTATUS_WPRI_4_MSB = 62;
parameter int MSTATUS_MSTATUS_WPRI_4_LSB = 40;
parameter int MSTATUS_MSTATUS_WPRI_4_WIDTH = 23;
parameter logic [22:0] MSTATUS_MSTATUS_WPRI_4_RESET = 64'h0000000000000000[62:40];
parameter logic [63:0] MSTATUS_MSTATUS_WPRI_4_MASK = 64'h7FFFFF0000000000;
parameter string MSTATUS_MSTATUS_WPRI_4_SW_TYPE = "WPRI";

parameter int MSTATUS_MPV_MSB = 39;
parameter int MSTATUS_MPV_LSB = 39;
parameter int MSTATUS_MPV_WIDTH = 1;
parameter logic [0:0] MSTATUS_MPV_RESET = 64'h0000000000000000[39:39];
parameter logic [63:0] MSTATUS_MPV_MASK = 64'h0000008000000000;
parameter string MSTATUS_MPV_SW_TYPE = "WARL";

parameter int MSTATUS_GVA_MSB = 38;
parameter int MSTATUS_GVA_LSB = 38;
parameter int MSTATUS_GVA_WIDTH = 1;
parameter logic [0:0] MSTATUS_GVA_RESET = 64'h0000000000000000[38:38];
parameter logic [63:0] MSTATUS_GVA_MASK = 64'h0000004000000000;
parameter string MSTATUS_GVA_SW_TYPE = "WARL";

parameter int MSTATUS_MBE_MSB = 37;
parameter int MSTATUS_MBE_LSB = 37;
parameter int MSTATUS_MBE_WIDTH = 1;
parameter logic [0:0] MSTATUS_MBE_RESET = 64'h0000000000000000[37:37];
parameter logic [63:0] MSTATUS_MBE_MASK = 64'h0000002000000000;
parameter string MSTATUS_MBE_SW_TYPE = "WARL";

parameter int MSTATUS_SBE_MSB = 36;
parameter int MSTATUS_SBE_LSB = 36;
parameter int MSTATUS_SBE_WIDTH = 1;
parameter logic [0:0] MSTATUS_SBE_RESET = 64'h0000000000000000[36:36];
parameter logic [63:0] MSTATUS_SBE_MASK = 64'h0000001000000000;
parameter string MSTATUS_SBE_SW_TYPE = "WARL";

parameter int MSTATUS_SXL_MSB = 35;
parameter int MSTATUS_SXL_LSB = 34;
parameter int MSTATUS_SXL_WIDTH = 2;
parameter logic [1:0] MSTATUS_SXL_RESET = 64'h0000000000000002[35:34];
parameter logic [63:0] MSTATUS_SXL_MASK = 64'h0000000C00000000;
parameter string MSTATUS_SXL_SW_TYPE = "WARL";

parameter int MSTATUS_UXL_MSB = 33;
parameter int MSTATUS_UXL_LSB = 32;
parameter int MSTATUS_UXL_WIDTH = 2;
parameter logic [1:0] MSTATUS_UXL_RESET = 64'h0000000000000002[33:32];
parameter logic [63:0] MSTATUS_UXL_MASK = 64'h0000000300000000;
parameter string MSTATUS_UXL_SW_TYPE = "WARL";

parameter int MSTATUS_MSTATUS_WPRI_3_MSB = 31;
parameter int MSTATUS_MSTATUS_WPRI_3_LSB = 23;
parameter int MSTATUS_MSTATUS_WPRI_3_WIDTH = 9;
parameter logic [8:0] MSTATUS_MSTATUS_WPRI_3_RESET = 64'h0000000000000000[31:23];
parameter logic [63:0] MSTATUS_MSTATUS_WPRI_3_MASK = 64'h00000000FF800000;
parameter string MSTATUS_MSTATUS_WPRI_3_SW_TYPE = "WPRI";

parameter int MSTATUS_TSR_MSB = 22;
parameter int MSTATUS_TSR_LSB = 22;
parameter int MSTATUS_TSR_WIDTH = 1;
parameter logic [0:0] MSTATUS_TSR_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] MSTATUS_TSR_MASK = 64'h0000000000400000;
parameter string MSTATUS_TSR_SW_TYPE = "WARL";

parameter int MSTATUS_TW_MSB = 21;
parameter int MSTATUS_TW_LSB = 21;
parameter int MSTATUS_TW_WIDTH = 1;
parameter logic [0:0] MSTATUS_TW_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] MSTATUS_TW_MASK = 64'h0000000000200000;
parameter string MSTATUS_TW_SW_TYPE = "WARL";

parameter int MSTATUS_TVM_MSB = 20;
parameter int MSTATUS_TVM_LSB = 20;
parameter int MSTATUS_TVM_WIDTH = 1;
parameter logic [0:0] MSTATUS_TVM_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] MSTATUS_TVM_MASK = 64'h0000000000100000;
parameter string MSTATUS_TVM_SW_TYPE = "WARL";

parameter int MSTATUS_MXR_MSB = 19;
parameter int MSTATUS_MXR_LSB = 19;
parameter int MSTATUS_MXR_WIDTH = 1;
parameter logic [0:0] MSTATUS_MXR_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] MSTATUS_MXR_MASK = 64'h0000000000080000;
parameter string MSTATUS_MXR_SW_TYPE = "WARL";

parameter int MSTATUS_SUM_MSB = 18;
parameter int MSTATUS_SUM_LSB = 18;
parameter int MSTATUS_SUM_WIDTH = 1;
parameter logic [0:0] MSTATUS_SUM_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] MSTATUS_SUM_MASK = 64'h0000000000040000;
parameter string MSTATUS_SUM_SW_TYPE = "WARL";

parameter int MSTATUS_MPRV_MSB = 17;
parameter int MSTATUS_MPRV_LSB = 17;
parameter int MSTATUS_MPRV_WIDTH = 1;
parameter logic [0:0] MSTATUS_MPRV_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] MSTATUS_MPRV_MASK = 64'h0000000000020000;
parameter string MSTATUS_MPRV_SW_TYPE = "WARL";

parameter int MSTATUS_XS_MSB = 16;
parameter int MSTATUS_XS_LSB = 15;
parameter int MSTATUS_XS_WIDTH = 2;
parameter logic [1:0] MSTATUS_XS_RESET = 64'h0000000000000000[16:15];
parameter logic [63:0] MSTATUS_XS_MASK = 64'h0000000000018000;
parameter string MSTATUS_XS_SW_TYPE = "WARL";

parameter int MSTATUS_FS_MSB = 14;
parameter int MSTATUS_FS_LSB = 13;
parameter int MSTATUS_FS_WIDTH = 2;
parameter logic [1:0] MSTATUS_FS_RESET = 64'h0000000000000000[14:13];
parameter logic [63:0] MSTATUS_FS_MASK = 64'h0000000000006000;
parameter string MSTATUS_FS_SW_TYPE = "WARL";

parameter int MSTATUS_MPP_MSB = 12;
parameter int MSTATUS_MPP_LSB = 11;
parameter int MSTATUS_MPP_WIDTH = 2;
parameter logic [1:0] MSTATUS_MPP_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] MSTATUS_MPP_MASK = 64'h0000000000001800;
parameter string MSTATUS_MPP_SW_TYPE = "WARL";

parameter int MSTATUS_VS_MSB = 10;
parameter int MSTATUS_VS_LSB = 9;
parameter int MSTATUS_VS_WIDTH = 2;
parameter logic [1:0] MSTATUS_VS_RESET = 64'h0000000000000000[10:9];
parameter logic [63:0] MSTATUS_VS_MASK = 64'h0000000000000600;
parameter string MSTATUS_VS_SW_TYPE = "WARL";

parameter int MSTATUS_SPP_MSB = 8;
parameter int MSTATUS_SPP_LSB = 8;
parameter int MSTATUS_SPP_WIDTH = 1;
parameter logic [0:0] MSTATUS_SPP_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] MSTATUS_SPP_MASK = 64'h0000000000000100;
parameter string MSTATUS_SPP_SW_TYPE = "WARL";

parameter int MSTATUS_MPIE_MSB = 7;
parameter int MSTATUS_MPIE_LSB = 7;
parameter int MSTATUS_MPIE_WIDTH = 1;
parameter logic [0:0] MSTATUS_MPIE_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MSTATUS_MPIE_MASK = 64'h0000000000000080;
parameter string MSTATUS_MPIE_SW_TYPE = "WARL";

parameter int MSTATUS_UBE_MSB = 6;
parameter int MSTATUS_UBE_LSB = 6;
parameter int MSTATUS_UBE_WIDTH = 1;
parameter logic [0:0] MSTATUS_UBE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MSTATUS_UBE_MASK = 64'h0000000000000040;
parameter string MSTATUS_UBE_SW_TYPE = "WARL";

parameter int MSTATUS_SPIE_MSB = 5;
parameter int MSTATUS_SPIE_LSB = 5;
parameter int MSTATUS_SPIE_WIDTH = 1;
parameter logic [0:0] MSTATUS_SPIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MSTATUS_SPIE_MASK = 64'h0000000000000020;
parameter string MSTATUS_SPIE_SW_TYPE = "WARL";

parameter int MSTATUS_MSTATUS_WPRI_2_MSB = 4;
parameter int MSTATUS_MSTATUS_WPRI_2_LSB = 4;
parameter int MSTATUS_MSTATUS_WPRI_2_WIDTH = 1;
parameter logic [0:0] MSTATUS_MSTATUS_WPRI_2_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] MSTATUS_MSTATUS_WPRI_2_MASK = 64'h0000000000000010;
parameter string MSTATUS_MSTATUS_WPRI_2_SW_TYPE = "WPRI";

parameter int MSTATUS_MIE_MSB = 3;
parameter int MSTATUS_MIE_LSB = 3;
parameter int MSTATUS_MIE_WIDTH = 1;
parameter logic [0:0] MSTATUS_MIE_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MSTATUS_MIE_MASK = 64'h0000000000000008;
parameter string MSTATUS_MIE_SW_TYPE = "WARL";

parameter int MSTATUS_MSTATUS_WPRI_1_MSB = 2;
parameter int MSTATUS_MSTATUS_WPRI_1_LSB = 2;
parameter int MSTATUS_MSTATUS_WPRI_1_WIDTH = 1;
parameter logic [0:0] MSTATUS_MSTATUS_WPRI_1_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MSTATUS_MSTATUS_WPRI_1_MASK = 64'h0000000000000004;
parameter string MSTATUS_MSTATUS_WPRI_1_SW_TYPE = "WPRI";

parameter int MSTATUS_SIE_MSB = 1;
parameter int MSTATUS_SIE_LSB = 1;
parameter int MSTATUS_SIE_WIDTH = 1;
parameter logic [0:0] MSTATUS_SIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MSTATUS_SIE_MASK = 64'h0000000000000002;
parameter string MSTATUS_SIE_SW_TYPE = "WARL";

parameter int MSTATUS_MSTATUS_WPRI_0_MSB = 0;
parameter int MSTATUS_MSTATUS_WPRI_0_LSB = 0;
parameter int MSTATUS_MSTATUS_WPRI_0_WIDTH = 1;
parameter logic [0:0] MSTATUS_MSTATUS_WPRI_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MSTATUS_MSTATUS_WPRI_0_MASK = 64'h0000000000000001;
parameter string MSTATUS_MSTATUS_WPRI_0_SW_TYPE = "WPRI";


// MTVEC CSR Field Defines
parameter int MTVEC_BASE_MSB = 63;
parameter int MTVEC_BASE_LSB = 2;
parameter int MTVEC_BASE_WIDTH = 62;
parameter logic [61:0] MTVEC_BASE_RESET = 64'h0000000000000000[63:2];
parameter logic [63:0] MTVEC_BASE_MASK = 64'hFFFFFFFFFFFFFFFC;
parameter string MTVEC_BASE_SW_TYPE = "WARL";

parameter int MTVEC_MODE_1_MSB = 1;
parameter int MTVEC_MODE_1_LSB = 1;
parameter int MTVEC_MODE_1_WIDTH = 1;
parameter logic [0:0] MTVEC_MODE_1_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MTVEC_MODE_1_MASK = 64'h0000000000000002;
parameter string MTVEC_MODE_1_SW_TYPE = "WARL";

parameter int MTVEC_MODE_0_MSB = 0;
parameter int MTVEC_MODE_0_LSB = 0;
parameter int MTVEC_MODE_0_WIDTH = 1;
parameter logic [0:0] MTVEC_MODE_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MTVEC_MODE_0_MASK = 64'h0000000000000001;
parameter string MTVEC_MODE_0_SW_TYPE = "WARL";


// MEDELEG CSR Field Defines
parameter int MEDELEG_RSVD_2_MSB = 63;
parameter int MEDELEG_RSVD_2_LSB = 24;
parameter int MEDELEG_RSVD_2_WIDTH = 40;
parameter logic [39:0] MEDELEG_RSVD_2_RESET = 64'h0000000000000000[63:24];
parameter logic [63:0] MEDELEG_RSVD_2_MASK = 64'hFFFFFFFFFF000000;
parameter string MEDELEG_RSVD_2_SW_TYPE = "WARL";

parameter int MEDELEG_MEDELEG_3_MSB = 23;
parameter int MEDELEG_MEDELEG_3_LSB = 20;
parameter int MEDELEG_MEDELEG_3_WIDTH = 4;
parameter logic [3:0] MEDELEG_MEDELEG_3_RESET = 64'h0000000000000000[23:20];
parameter logic [63:0] MEDELEG_MEDELEG_3_MASK = 64'h0000000000F00000;
parameter string MEDELEG_MEDELEG_3_SW_TYPE = "WARL";

parameter int MEDELEG_RSVD_1_MSB = 19;
parameter int MEDELEG_RSVD_1_LSB = 16;
parameter int MEDELEG_RSVD_1_WIDTH = 4;
parameter logic [3:0] MEDELEG_RSVD_1_RESET = 64'h0000000000000000[19:16];
parameter logic [63:0] MEDELEG_RSVD_1_MASK = 64'h00000000000F0000;
parameter string MEDELEG_RSVD_1_SW_TYPE = "WARL";

parameter int MEDELEG_MEDELEG_2_MSB = 15;
parameter int MEDELEG_MEDELEG_2_LSB = 15;
parameter int MEDELEG_MEDELEG_2_WIDTH = 1;
parameter logic [0:0] MEDELEG_MEDELEG_2_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] MEDELEG_MEDELEG_2_MASK = 64'h0000000000008000;
parameter string MEDELEG_MEDELEG_2_SW_TYPE = "WARL";

parameter int MEDELEG_RSVD_0_MSB = 14;
parameter int MEDELEG_RSVD_0_LSB = 14;
parameter int MEDELEG_RSVD_0_WIDTH = 1;
parameter logic [0:0] MEDELEG_RSVD_0_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] MEDELEG_RSVD_0_MASK = 64'h0000000000004000;
parameter string MEDELEG_RSVD_0_SW_TYPE = "WARL";

parameter int MEDELEG_MEDELEG_1_MSB = 13;
parameter int MEDELEG_MEDELEG_1_LSB = 12;
parameter int MEDELEG_MEDELEG_1_WIDTH = 2;
parameter logic [1:0] MEDELEG_MEDELEG_1_RESET = 64'h0000000000000000[13:12];
parameter logic [63:0] MEDELEG_MEDELEG_1_MASK = 64'h0000000000003000;
parameter string MEDELEG_MEDELEG_1_SW_TYPE = "WARL";

parameter int MEDELEG_ECALL_FROM_M_MSB = 11;
parameter int MEDELEG_ECALL_FROM_M_LSB = 11;
parameter int MEDELEG_ECALL_FROM_M_WIDTH = 1;
parameter logic [0:0] MEDELEG_ECALL_FROM_M_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MEDELEG_ECALL_FROM_M_MASK = 64'h0000000000000800;
parameter string MEDELEG_ECALL_FROM_M_SW_TYPE = "WARL";

parameter int MEDELEG_MEDELEG_MASKED_0_MSB = 10;
parameter int MEDELEG_MEDELEG_MASKED_0_LSB = 10;
parameter int MEDELEG_MEDELEG_MASKED_0_WIDTH = 1;
parameter logic [0:0] MEDELEG_MEDELEG_MASKED_0_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MEDELEG_MEDELEG_MASKED_0_MASK = 64'h0000000000000400;
parameter string MEDELEG_MEDELEG_MASKED_0_SW_TYPE = "WARL";

parameter int MEDELEG_MEDELEG_0_MSB = 9;
parameter int MEDELEG_MEDELEG_0_LSB = 0;
parameter int MEDELEG_MEDELEG_0_WIDTH = 10;
parameter logic [9:0] MEDELEG_MEDELEG_0_RESET = 64'h0000000000000000[9:0];
parameter logic [63:0] MEDELEG_MEDELEG_0_MASK = 64'h00000000000003FF;
parameter string MEDELEG_MEDELEG_0_SW_TYPE = "WARL";


// MIDELEG CSR Field Defines
parameter int MIDELEG_LCOFIP_MSB = 13;
parameter int MIDELEG_LCOFIP_LSB = 13;
parameter int MIDELEG_LCOFIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MIDELEG_LCOFIP_MASK = 64'h0000000000002000;
parameter string MIDELEG_LCOFIP_SW_TYPE = "WARL";

parameter int MIDELEG_SGEIP_MSB = 12;
parameter int MIDELEG_SGEIP_LSB = 12;
parameter int MIDELEG_SGEIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_SGEIP_RESET = 64'h0000000000000001[12:12];
parameter logic [63:0] MIDELEG_SGEIP_MASK = 64'h0000000000001000;
parameter string MIDELEG_SGEIP_SW_TYPE = "WARL";

parameter int MIDELEG_MEIP_MSB = 11;
parameter int MIDELEG_MEIP_LSB = 11;
parameter int MIDELEG_MEIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_MEIP_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MIDELEG_MEIP_MASK = 64'h0000000000000800;
parameter string MIDELEG_MEIP_SW_TYPE = "WARL";

parameter int MIDELEG_VSEIP_MSB = 10;
parameter int MIDELEG_VSEIP_LSB = 10;
parameter int MIDELEG_VSEIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_VSEIP_RESET = 64'h0000000000000001[10:10];
parameter logic [63:0] MIDELEG_VSEIP_MASK = 64'h0000000000000400;
parameter string MIDELEG_VSEIP_SW_TYPE = "WARL";

parameter int MIDELEG_SEIP_MSB = 9;
parameter int MIDELEG_SEIP_LSB = 9;
parameter int MIDELEG_SEIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_SEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MIDELEG_SEIP_MASK = 64'h0000000000000200;
parameter string MIDELEG_SEIP_SW_TYPE = "WARL";

parameter int MIDELEG_MTIP_MSB = 7;
parameter int MIDELEG_MTIP_LSB = 7;
parameter int MIDELEG_MTIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_MTIP_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MIDELEG_MTIP_MASK = 64'h0000000000000080;
parameter string MIDELEG_MTIP_SW_TYPE = "WARL";

parameter int MIDELEG_VSTIP_MSB = 6;
parameter int MIDELEG_VSTIP_LSB = 6;
parameter int MIDELEG_VSTIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_VSTIP_RESET = 64'h0000000000000001[6:6];
parameter logic [63:0] MIDELEG_VSTIP_MASK = 64'h0000000000000040;
parameter string MIDELEG_VSTIP_SW_TYPE = "WARL";

parameter int MIDELEG_STIP_MSB = 5;
parameter int MIDELEG_STIP_LSB = 5;
parameter int MIDELEG_STIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_STIP_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MIDELEG_STIP_MASK = 64'h0000000000000020;
parameter string MIDELEG_STIP_SW_TYPE = "WARL";

parameter int MIDELEG_MSIP_MSB = 3;
parameter int MIDELEG_MSIP_LSB = 3;
parameter int MIDELEG_MSIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_MSIP_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MIDELEG_MSIP_MASK = 64'h0000000000000008;
parameter string MIDELEG_MSIP_SW_TYPE = "WARL";

parameter int MIDELEG_VSSIP_MSB = 2;
parameter int MIDELEG_VSSIP_LSB = 2;
parameter int MIDELEG_VSSIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_VSSIP_RESET = 64'h0000000000000001[2:2];
parameter logic [63:0] MIDELEG_VSSIP_MASK = 64'h0000000000000004;
parameter string MIDELEG_VSSIP_SW_TYPE = "WARL";

parameter int MIDELEG_SSIP_MSB = 1;
parameter int MIDELEG_SSIP_LSB = 1;
parameter int MIDELEG_SSIP_WIDTH = 1;
parameter logic [0:0] MIDELEG_SSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MIDELEG_SSIP_MASK = 64'h0000000000000002;
parameter string MIDELEG_SSIP_SW_TYPE = "WARL";


// MIP CSR Field Defines
parameter int MIP_NONSTANDARDINTERRUPTS_MSB = 63;
parameter int MIP_NONSTANDARDINTERRUPTS_LSB = 16;
parameter int MIP_NONSTANDARDINTERRUPTS_WIDTH = 48;
parameter logic [47:0] MIP_NONSTANDARDINTERRUPTS_RESET = 64'h0000000000000000[63:16];
parameter logic [63:0] MIP_NONSTANDARDINTERRUPTS_MASK = 64'hFFFFFFFFFFFF0000;
parameter string MIP_NONSTANDARDINTERRUPTS_SW_TYPE = "WARL";

parameter int MIP_LCOFIP_MSB = 13;
parameter int MIP_LCOFIP_LSB = 13;
parameter int MIP_LCOFIP_WIDTH = 1;
parameter logic [0:0] MIP_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MIP_LCOFIP_MASK = 64'h0000000000002000;
parameter string MIP_LCOFIP_SW_TYPE = "WARL";

parameter int MIP_SGEIP_MSB = 12;
parameter int MIP_SGEIP_LSB = 12;
parameter int MIP_SGEIP_WIDTH = 1;
parameter logic [0:0] MIP_SGEIP_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] MIP_SGEIP_MASK = 64'h0000000000001000;
parameter string MIP_SGEIP_SW_TYPE = "WARL";

parameter int MIP_MEIP_MSB = 11;
parameter int MIP_MEIP_LSB = 11;
parameter int MIP_MEIP_WIDTH = 1;
parameter logic [0:0] MIP_MEIP_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MIP_MEIP_MASK = 64'h0000000000000800;
parameter string MIP_MEIP_SW_TYPE = "WARL";

parameter int MIP_VSEIP_MSB = 10;
parameter int MIP_VSEIP_LSB = 10;
parameter int MIP_VSEIP_WIDTH = 1;
parameter logic [0:0] MIP_VSEIP_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MIP_VSEIP_MASK = 64'h0000000000000400;
parameter string MIP_VSEIP_SW_TYPE = "WARL";

parameter int MIP_SEIP_MSB = 9;
parameter int MIP_SEIP_LSB = 9;
parameter int MIP_SEIP_WIDTH = 1;
parameter logic [0:0] MIP_SEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MIP_SEIP_MASK = 64'h0000000000000200;
parameter string MIP_SEIP_SW_TYPE = "WARL";

parameter int MIP_MTIP_MSB = 7;
parameter int MIP_MTIP_LSB = 7;
parameter int MIP_MTIP_WIDTH = 1;
parameter logic [0:0] MIP_MTIP_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MIP_MTIP_MASK = 64'h0000000000000080;
parameter string MIP_MTIP_SW_TYPE = "WARL";

parameter int MIP_VSTIP_MSB = 6;
parameter int MIP_VSTIP_LSB = 6;
parameter int MIP_VSTIP_WIDTH = 1;
parameter logic [0:0] MIP_VSTIP_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MIP_VSTIP_MASK = 64'h0000000000000040;
parameter string MIP_VSTIP_SW_TYPE = "WARL";

parameter int MIP_STIP_MSB = 5;
parameter int MIP_STIP_LSB = 5;
parameter int MIP_STIP_WIDTH = 1;
parameter logic [0:0] MIP_STIP_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MIP_STIP_MASK = 64'h0000000000000020;
parameter string MIP_STIP_SW_TYPE = "WARL";

parameter int MIP_MSIP_MSB = 3;
parameter int MIP_MSIP_LSB = 3;
parameter int MIP_MSIP_WIDTH = 1;
parameter logic [0:0] MIP_MSIP_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MIP_MSIP_MASK = 64'h0000000000000008;
parameter string MIP_MSIP_SW_TYPE = "WARL";

parameter int MIP_VSSIP_MSB = 2;
parameter int MIP_VSSIP_LSB = 2;
parameter int MIP_VSSIP_WIDTH = 1;
parameter logic [0:0] MIP_VSSIP_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MIP_VSSIP_MASK = 64'h0000000000000004;
parameter string MIP_VSSIP_SW_TYPE = "WARL";

parameter int MIP_SSIP_MSB = 1;
parameter int MIP_SSIP_LSB = 1;
parameter int MIP_SSIP_WIDTH = 1;
parameter logic [0:0] MIP_SSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MIP_SSIP_MASK = 64'h0000000000000002;
parameter string MIP_SSIP_SW_TYPE = "WARL";


// MIE CSR Field Defines
parameter int MIE_NONSTANDARDINTERRUPTS_MSB = 63;
parameter int MIE_NONSTANDARDINTERRUPTS_LSB = 16;
parameter int MIE_NONSTANDARDINTERRUPTS_WIDTH = 48;
parameter logic [47:0] MIE_NONSTANDARDINTERRUPTS_RESET = 64'h0000000000000000[63:16];
parameter logic [63:0] MIE_NONSTANDARDINTERRUPTS_MASK = 64'hFFFFFFFFFFFF0000;
parameter string MIE_NONSTANDARDINTERRUPTS_SW_TYPE = "WARL";

parameter int MIE_LCOFIE_MSB = 13;
parameter int MIE_LCOFIE_LSB = 13;
parameter int MIE_LCOFIE_WIDTH = 1;
parameter logic [0:0] MIE_LCOFIE_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MIE_LCOFIE_MASK = 64'h0000000000002000;
parameter string MIE_LCOFIE_SW_TYPE = "WARL";

parameter int MIE_SGEIE_MSB = 12;
parameter int MIE_SGEIE_LSB = 12;
parameter int MIE_SGEIE_WIDTH = 1;
parameter logic [0:0] MIE_SGEIE_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] MIE_SGEIE_MASK = 64'h0000000000001000;
parameter string MIE_SGEIE_SW_TYPE = "WARL";

parameter int MIE_MEIE_MSB = 11;
parameter int MIE_MEIE_LSB = 11;
parameter int MIE_MEIE_WIDTH = 1;
parameter logic [0:0] MIE_MEIE_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MIE_MEIE_MASK = 64'h0000000000000800;
parameter string MIE_MEIE_SW_TYPE = "WARL";

parameter int MIE_VSEIE_MSB = 10;
parameter int MIE_VSEIE_LSB = 10;
parameter int MIE_VSEIE_WIDTH = 1;
parameter logic [0:0] MIE_VSEIE_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MIE_VSEIE_MASK = 64'h0000000000000400;
parameter string MIE_VSEIE_SW_TYPE = "WARL";

parameter int MIE_SEIE_MSB = 9;
parameter int MIE_SEIE_LSB = 9;
parameter int MIE_SEIE_WIDTH = 1;
parameter logic [0:0] MIE_SEIE_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MIE_SEIE_MASK = 64'h0000000000000200;
parameter string MIE_SEIE_SW_TYPE = "WARL";

parameter int MIE_MTIE_MSB = 7;
parameter int MIE_MTIE_LSB = 7;
parameter int MIE_MTIE_WIDTH = 1;
parameter logic [0:0] MIE_MTIE_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MIE_MTIE_MASK = 64'h0000000000000080;
parameter string MIE_MTIE_SW_TYPE = "WARL";

parameter int MIE_VSTIE_MSB = 6;
parameter int MIE_VSTIE_LSB = 6;
parameter int MIE_VSTIE_WIDTH = 1;
parameter logic [0:0] MIE_VSTIE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MIE_VSTIE_MASK = 64'h0000000000000040;
parameter string MIE_VSTIE_SW_TYPE = "WARL";

parameter int MIE_STIE_MSB = 5;
parameter int MIE_STIE_LSB = 5;
parameter int MIE_STIE_WIDTH = 1;
parameter logic [0:0] MIE_STIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MIE_STIE_MASK = 64'h0000000000000020;
parameter string MIE_STIE_SW_TYPE = "WARL";

parameter int MIE_MSIE_MSB = 3;
parameter int MIE_MSIE_LSB = 3;
parameter int MIE_MSIE_WIDTH = 1;
parameter logic [0:0] MIE_MSIE_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MIE_MSIE_MASK = 64'h0000000000000008;
parameter string MIE_MSIE_SW_TYPE = "WARL";

parameter int MIE_VSSIE_MSB = 2;
parameter int MIE_VSSIE_LSB = 2;
parameter int MIE_VSSIE_WIDTH = 1;
parameter logic [0:0] MIE_VSSIE_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MIE_VSSIE_MASK = 64'h0000000000000004;
parameter string MIE_VSSIE_SW_TYPE = "WARL";

parameter int MIE_SSIE_MSB = 1;
parameter int MIE_SSIE_LSB = 1;
parameter int MIE_SSIE_WIDTH = 1;
parameter logic [0:0] MIE_SSIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MIE_SSIE_MASK = 64'h0000000000000002;
parameter string MIE_SSIE_SW_TYPE = "WARL";


// MSCRATCH CSR Field Defines
parameter int MSCRATCH_MSCRATCH_MSB = 63;
parameter int MSCRATCH_MSCRATCH_LSB = 0;
parameter int MSCRATCH_MSCRATCH_WIDTH = 64;
parameter logic [63:0] MSCRATCH_MSCRATCH_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MSCRATCH_MSCRATCH_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MSCRATCH_MSCRATCH_SW_TYPE = "WARL";


// MEPC CSR Field Defines
parameter int MEPC_ADDR_MSB = 63;
parameter int MEPC_ADDR_LSB = 1;
parameter int MEPC_ADDR_WIDTH = 63;
parameter logic [62:0] MEPC_ADDR_RESET = 64'h0000000000000000[63:1];
parameter logic [63:0] MEPC_ADDR_MASK = 64'hFFFFFFFFFFFFFFFE;
parameter string MEPC_ADDR_SW_TYPE = "WARL";


// MCAUSE CSR Field Defines
parameter int MCAUSE_INTERRUPT_MSB = 63;
parameter int MCAUSE_INTERRUPT_LSB = 63;
parameter int MCAUSE_INTERRUPT_WIDTH = 1;
parameter logic [0:0] MCAUSE_INTERRUPT_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MCAUSE_INTERRUPT_MASK = 64'h8000000000000000;
parameter string MCAUSE_INTERRUPT_SW_TYPE = "WARL";

parameter int MCAUSE_EXCEPTIONCODEWLRL_MSB = 62;
parameter int MCAUSE_EXCEPTIONCODEWLRL_LSB = 0;
parameter int MCAUSE_EXCEPTIONCODEWLRL_WIDTH = 63;
parameter logic [62:0] MCAUSE_EXCEPTIONCODEWLRL_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] MCAUSE_EXCEPTIONCODEWLRL_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string MCAUSE_EXCEPTIONCODEWLRL_SW_TYPE = "WARL";


// MTVAL CSR Field Defines
parameter int MTVAL_MTVAL_MSB = 63;
parameter int MTVAL_MTVAL_LSB = 0;
parameter int MTVAL_MTVAL_WIDTH = 64;
parameter logic [63:0] MTVAL_MTVAL_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MTVAL_MTVAL_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MTVAL_MTVAL_SW_TYPE = "WARL";


// MCONFIGPTR CSR Field Defines
parameter int MCONFIGPTR_MCONFIGPTR_MSB = 63;
parameter int MCONFIGPTR_MCONFIGPTR_LSB = 0;
parameter int MCONFIGPTR_MCONFIGPTR_WIDTH = 64;
parameter logic [63:0] MCONFIGPTR_MCONFIGPTR_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MCONFIGPTR_MCONFIGPTR_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MCONFIGPTR_MCONFIGPTR_SW_TYPE = "WARL";


// MENVCFG CSR Field Defines
parameter int MENVCFG_STCE_MSB = 63;
parameter int MENVCFG_STCE_LSB = 63;
parameter int MENVCFG_STCE_WIDTH = 1;
parameter logic [0:0] MENVCFG_STCE_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MENVCFG_STCE_MASK = 64'h8000000000000000;
parameter string MENVCFG_STCE_SW_TYPE = "WARL";

parameter int MENVCFG_PBMTE_MSB = 62;
parameter int MENVCFG_PBMTE_LSB = 62;
parameter int MENVCFG_PBMTE_WIDTH = 1;
parameter logic [0:0] MENVCFG_PBMTE_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MENVCFG_PBMTE_MASK = 64'h4000000000000000;
parameter string MENVCFG_PBMTE_SW_TYPE = "WARL";

parameter int MENVCFG_HADE_MSB = 61;
parameter int MENVCFG_HADE_LSB = 61;
parameter int MENVCFG_HADE_WIDTH = 1;
parameter logic [0:0] MENVCFG_HADE_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MENVCFG_HADE_MASK = 64'h2000000000000000;
parameter string MENVCFG_HADE_SW_TYPE = "WARL";

parameter int MENVCFG_WPRI_2_MSB = 60;
parameter int MENVCFG_WPRI_2_LSB = 34;
parameter int MENVCFG_WPRI_2_WIDTH = 27;
parameter logic [26:0] MENVCFG_WPRI_2_RESET = 64'h0000000000000000[60:34];
parameter logic [63:0] MENVCFG_WPRI_2_MASK = 64'h1FFFFFFC00000000;
parameter string MENVCFG_WPRI_2_SW_TYPE = "WPRI";

parameter int MENVCFG_PMM_MSB = 33;
parameter int MENVCFG_PMM_LSB = 32;
parameter int MENVCFG_PMM_WIDTH = 2;
parameter logic [1:0] MENVCFG_PMM_RESET = 64'h0000000000000000[33:32];
parameter logic [63:0] MENVCFG_PMM_MASK = 64'h0000000300000000;
parameter string MENVCFG_PMM_SW_TYPE = "WARL";

parameter int MENVCFG_WPRI_1_MSB = 31;
parameter int MENVCFG_WPRI_1_LSB = 8;
parameter int MENVCFG_WPRI_1_WIDTH = 24;
parameter logic [23:0] MENVCFG_WPRI_1_RESET = 64'h0000000000000000[31:8];
parameter logic [63:0] MENVCFG_WPRI_1_MASK = 64'h00000000FFFFFF00;
parameter string MENVCFG_WPRI_1_SW_TYPE = "WPRI";

parameter int MENVCFG_CBZE_MSB = 7;
parameter int MENVCFG_CBZE_LSB = 7;
parameter int MENVCFG_CBZE_WIDTH = 1;
parameter logic [0:0] MENVCFG_CBZE_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MENVCFG_CBZE_MASK = 64'h0000000000000080;
parameter string MENVCFG_CBZE_SW_TYPE = "WARL";

parameter int MENVCFG_CBCFE_MSB = 6;
parameter int MENVCFG_CBCFE_LSB = 6;
parameter int MENVCFG_CBCFE_WIDTH = 1;
parameter logic [0:0] MENVCFG_CBCFE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MENVCFG_CBCFE_MASK = 64'h0000000000000040;
parameter string MENVCFG_CBCFE_SW_TYPE = "WARL";

parameter int MENVCFG_CBIE_MSB = 5;
parameter int MENVCFG_CBIE_LSB = 4;
parameter int MENVCFG_CBIE_WIDTH = 2;
parameter logic [1:0] MENVCFG_CBIE_RESET = 64'h0000000000000000[5:4];
parameter logic [63:0] MENVCFG_CBIE_MASK = 64'h0000000000000030;
parameter string MENVCFG_CBIE_SW_TYPE = "WARL";

parameter int MENVCFG_WPRI_0_MSB = 3;
parameter int MENVCFG_WPRI_0_LSB = 1;
parameter int MENVCFG_WPRI_0_WIDTH = 3;
parameter logic [2:0] MENVCFG_WPRI_0_RESET = 64'h0000000000000000[3:1];
parameter logic [63:0] MENVCFG_WPRI_0_MASK = 64'h000000000000000E;
parameter string MENVCFG_WPRI_0_SW_TYPE = "WPRI";

parameter int MENVCFG_FIOM_MSB = 0;
parameter int MENVCFG_FIOM_LSB = 0;
parameter int MENVCFG_FIOM_WIDTH = 1;
parameter logic [0:0] MENVCFG_FIOM_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MENVCFG_FIOM_MASK = 64'h0000000000000001;
parameter string MENVCFG_FIOM_SW_TYPE = "WARL";


// MSECCFG CSR Field Defines
parameter int MSECCFG_WPRI_2_MSB = 63;
parameter int MSECCFG_WPRI_2_LSB = 34;
parameter int MSECCFG_WPRI_2_WIDTH = 30;
parameter logic [29:0] MSECCFG_WPRI_2_RESET = 64'h0000000000000000[63:34];
parameter logic [63:0] MSECCFG_WPRI_2_MASK = 64'hFFFFFFFC00000000;
parameter string MSECCFG_WPRI_2_SW_TYPE = "WPRI";

parameter int MSECCFG_PMM_MSB = 33;
parameter int MSECCFG_PMM_LSB = 32;
parameter int MSECCFG_PMM_WIDTH = 2;
parameter logic [1:0] MSECCFG_PMM_RESET = 64'h0000000000000000[33:32];
parameter logic [63:0] MSECCFG_PMM_MASK = 64'h0000000300000000;
parameter string MSECCFG_PMM_SW_TYPE = "WARL";

parameter int MSECCFG_WPRI_1_MSB = 31;
parameter int MSECCFG_WPRI_1_LSB = 10;
parameter int MSECCFG_WPRI_1_WIDTH = 22;
parameter logic [21:0] MSECCFG_WPRI_1_RESET = 64'h0000000000000000[31:10];
parameter logic [63:0] MSECCFG_WPRI_1_MASK = 64'h00000000FFFFFC00;
parameter string MSECCFG_WPRI_1_SW_TYPE = "WPRI";

parameter int MSECCFG_SSEED_MSB = 9;
parameter int MSECCFG_SSEED_LSB = 9;
parameter int MSECCFG_SSEED_WIDTH = 1;
parameter logic [0:0] MSECCFG_SSEED_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MSECCFG_SSEED_MASK = 64'h0000000000000200;
parameter string MSECCFG_SSEED_SW_TYPE = "WARL";

parameter int MSECCFG_USEED_MSB = 8;
parameter int MSECCFG_USEED_LSB = 8;
parameter int MSECCFG_USEED_WIDTH = 1;
parameter logic [0:0] MSECCFG_USEED_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] MSECCFG_USEED_MASK = 64'h0000000000000100;
parameter string MSECCFG_USEED_SW_TYPE = "WARL";

parameter int MSECCFG_WPRI_0_MSB = 7;
parameter int MSECCFG_WPRI_0_LSB = 3;
parameter int MSECCFG_WPRI_0_WIDTH = 5;
parameter logic [4:0] MSECCFG_WPRI_0_RESET = 64'h0000000000000000[7:3];
parameter logic [63:0] MSECCFG_WPRI_0_MASK = 64'h00000000000000F8;
parameter string MSECCFG_WPRI_0_SW_TYPE = "WPRI";

parameter int MSECCFG_RLB_MSB = 2;
parameter int MSECCFG_RLB_LSB = 2;
parameter int MSECCFG_RLB_WIDTH = 1;
parameter logic [0:0] MSECCFG_RLB_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MSECCFG_RLB_MASK = 64'h0000000000000004;
parameter string MSECCFG_RLB_SW_TYPE = "WARL";

parameter int MSECCFG_MMWP_MSB = 1;
parameter int MSECCFG_MMWP_LSB = 1;
parameter int MSECCFG_MMWP_WIDTH = 1;
parameter logic [0:0] MSECCFG_MMWP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MSECCFG_MMWP_MASK = 64'h0000000000000002;
parameter string MSECCFG_MMWP_SW_TYPE = "WARL";

parameter int MSECCFG_MML_MSB = 0;
parameter int MSECCFG_MML_LSB = 0;
parameter int MSECCFG_MML_WIDTH = 1;
parameter logic [0:0] MSECCFG_MML_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MSECCFG_MML_MASK = 64'h0000000000000001;
parameter string MSECCFG_MML_SW_TYPE = "WARL";


// MCYCLE CSR Field Defines
parameter int MCYCLE_CYCLE_MSB = 63;
parameter int MCYCLE_CYCLE_LSB = 0;
parameter int MCYCLE_CYCLE_WIDTH = 64;
parameter logic [63:0] MCYCLE_CYCLE_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MCYCLE_CYCLE_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MCYCLE_CYCLE_SW_TYPE = "WARL";


// MINSTRET CSR Field Defines
parameter int MINSTRET_INSTRET_MSB = 63;
parameter int MINSTRET_INSTRET_LSB = 0;
parameter int MINSTRET_INSTRET_WIDTH = 64;
parameter logic [63:0] MINSTRET_INSTRET_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MINSTRET_INSTRET_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MINSTRET_INSTRET_SW_TYPE = "WARL";


// MHPMCOUNTER3 CSR Field Defines
parameter int MHPMCOUNTER3_HPMCOUNTER3_MSB = 63;
parameter int MHPMCOUNTER3_HPMCOUNTER3_LSB = 0;
parameter int MHPMCOUNTER3_HPMCOUNTER3_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER3_HPMCOUNTER3_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER3_HPMCOUNTER3_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER3_HPMCOUNTER3_SW_TYPE = "WARL";


// MHPMCOUNTER4 CSR Field Defines
parameter int MHPMCOUNTER4_HPMCOUNTER4_MSB = 63;
parameter int MHPMCOUNTER4_HPMCOUNTER4_LSB = 0;
parameter int MHPMCOUNTER4_HPMCOUNTER4_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER4_HPMCOUNTER4_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER4_HPMCOUNTER4_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER4_HPMCOUNTER4_SW_TYPE = "WARL";


// MHPMCOUNTER5 CSR Field Defines
parameter int MHPMCOUNTER5_HPMCOUNTER5_MSB = 63;
parameter int MHPMCOUNTER5_HPMCOUNTER5_LSB = 0;
parameter int MHPMCOUNTER5_HPMCOUNTER5_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER5_HPMCOUNTER5_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER5_HPMCOUNTER5_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER5_HPMCOUNTER5_SW_TYPE = "WARL";


// MHPMCOUNTER6 CSR Field Defines
parameter int MHPMCOUNTER6_HPMCOUNTER6_MSB = 63;
parameter int MHPMCOUNTER6_HPMCOUNTER6_LSB = 0;
parameter int MHPMCOUNTER6_HPMCOUNTER6_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER6_HPMCOUNTER6_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER6_HPMCOUNTER6_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER6_HPMCOUNTER6_SW_TYPE = "WARL";


// MHPMCOUNTER7 CSR Field Defines
parameter int MHPMCOUNTER7_HPMCOUNTER7_MSB = 63;
parameter int MHPMCOUNTER7_HPMCOUNTER7_LSB = 0;
parameter int MHPMCOUNTER7_HPMCOUNTER7_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER7_HPMCOUNTER7_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER7_HPMCOUNTER7_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER7_HPMCOUNTER7_SW_TYPE = "WARL";


// MHPMCOUNTER8 CSR Field Defines
parameter int MHPMCOUNTER8_HPMCOUNTER8_MSB = 63;
parameter int MHPMCOUNTER8_HPMCOUNTER8_LSB = 0;
parameter int MHPMCOUNTER8_HPMCOUNTER8_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER8_HPMCOUNTER8_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER8_HPMCOUNTER8_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER8_HPMCOUNTER8_SW_TYPE = "WARL";


// MHPMCOUNTER9 CSR Field Defines
parameter int MHPMCOUNTER9_HPMCOUNTER9_MSB = 63;
parameter int MHPMCOUNTER9_HPMCOUNTER9_LSB = 0;
parameter int MHPMCOUNTER9_HPMCOUNTER9_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER9_HPMCOUNTER9_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER9_HPMCOUNTER9_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER9_HPMCOUNTER9_SW_TYPE = "WARL";


// MHPMCOUNTER10 CSR Field Defines
parameter int MHPMCOUNTER10_HPMCOUNTER10_MSB = 63;
parameter int MHPMCOUNTER10_HPMCOUNTER10_LSB = 0;
parameter int MHPMCOUNTER10_HPMCOUNTER10_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER10_HPMCOUNTER10_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER10_HPMCOUNTER10_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER10_HPMCOUNTER10_SW_TYPE = "WARL";


// MHPMCOUNTER11 CSR Field Defines
parameter int MHPMCOUNTER11_HPMCOUNTER11_MSB = 63;
parameter int MHPMCOUNTER11_HPMCOUNTER11_LSB = 0;
parameter int MHPMCOUNTER11_HPMCOUNTER11_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER11_HPMCOUNTER11_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER11_HPMCOUNTER11_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER11_HPMCOUNTER11_SW_TYPE = "WARL";


// MHPMCOUNTER12 CSR Field Defines
parameter int MHPMCOUNTER12_HPMCOUNTER12_MSB = 63;
parameter int MHPMCOUNTER12_HPMCOUNTER12_LSB = 0;
parameter int MHPMCOUNTER12_HPMCOUNTER12_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER12_HPMCOUNTER12_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER12_HPMCOUNTER12_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER12_HPMCOUNTER12_SW_TYPE = "WARL";


// MHPMCOUNTER13 CSR Field Defines
parameter int MHPMCOUNTER13_HPMCOUNTER13_MSB = 63;
parameter int MHPMCOUNTER13_HPMCOUNTER13_LSB = 0;
parameter int MHPMCOUNTER13_HPMCOUNTER13_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER13_HPMCOUNTER13_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER13_HPMCOUNTER13_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER13_HPMCOUNTER13_SW_TYPE = "WARL";


// MHPMCOUNTER14 CSR Field Defines
parameter int MHPMCOUNTER14_HPMCOUNTER14_MSB = 63;
parameter int MHPMCOUNTER14_HPMCOUNTER14_LSB = 0;
parameter int MHPMCOUNTER14_HPMCOUNTER14_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER14_HPMCOUNTER14_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER14_HPMCOUNTER14_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER14_HPMCOUNTER14_SW_TYPE = "WARL";


// MHPMCOUNTER15 CSR Field Defines
parameter int MHPMCOUNTER15_HPMCOUNTER15_MSB = 63;
parameter int MHPMCOUNTER15_HPMCOUNTER15_LSB = 0;
parameter int MHPMCOUNTER15_HPMCOUNTER15_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER15_HPMCOUNTER15_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER15_HPMCOUNTER15_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER15_HPMCOUNTER15_SW_TYPE = "WARL";


// MHPMCOUNTER16 CSR Field Defines
parameter int MHPMCOUNTER16_HPMCOUNTER16_MSB = 63;
parameter int MHPMCOUNTER16_HPMCOUNTER16_LSB = 0;
parameter int MHPMCOUNTER16_HPMCOUNTER16_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER16_HPMCOUNTER16_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER16_HPMCOUNTER16_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER16_HPMCOUNTER16_SW_TYPE = "WARL";


// MHPMCOUNTER17 CSR Field Defines
parameter int MHPMCOUNTER17_HPMCOUNTER17_MSB = 63;
parameter int MHPMCOUNTER17_HPMCOUNTER17_LSB = 0;
parameter int MHPMCOUNTER17_HPMCOUNTER17_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER17_HPMCOUNTER17_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER17_HPMCOUNTER17_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER17_HPMCOUNTER17_SW_TYPE = "WARL";


// MHPMCOUNTER18 CSR Field Defines
parameter int MHPMCOUNTER18_HPMCOUNTER18_MSB = 63;
parameter int MHPMCOUNTER18_HPMCOUNTER18_LSB = 0;
parameter int MHPMCOUNTER18_HPMCOUNTER18_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER18_HPMCOUNTER18_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER18_HPMCOUNTER18_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER18_HPMCOUNTER18_SW_TYPE = "WARL";


// MHPMCOUNTER19 CSR Field Defines
parameter int MHPMCOUNTER19_HPMCOUNTER19_MSB = 63;
parameter int MHPMCOUNTER19_HPMCOUNTER19_LSB = 0;
parameter int MHPMCOUNTER19_HPMCOUNTER19_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER19_HPMCOUNTER19_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER19_HPMCOUNTER19_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER19_HPMCOUNTER19_SW_TYPE = "WARL";


// MHPMCOUNTER20 CSR Field Defines
parameter int MHPMCOUNTER20_HPMCOUNTER20_MSB = 63;
parameter int MHPMCOUNTER20_HPMCOUNTER20_LSB = 0;
parameter int MHPMCOUNTER20_HPMCOUNTER20_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER20_HPMCOUNTER20_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER20_HPMCOUNTER20_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER20_HPMCOUNTER20_SW_TYPE = "WARL";


// MHPMCOUNTER21 CSR Field Defines
parameter int MHPMCOUNTER21_HPMCOUNTER21_MSB = 63;
parameter int MHPMCOUNTER21_HPMCOUNTER21_LSB = 0;
parameter int MHPMCOUNTER21_HPMCOUNTER21_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER21_HPMCOUNTER21_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER21_HPMCOUNTER21_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER21_HPMCOUNTER21_SW_TYPE = "WARL";


// MHPMCOUNTER22 CSR Field Defines
parameter int MHPMCOUNTER22_HPMCOUNTER22_MSB = 63;
parameter int MHPMCOUNTER22_HPMCOUNTER22_LSB = 0;
parameter int MHPMCOUNTER22_HPMCOUNTER22_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER22_HPMCOUNTER22_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER22_HPMCOUNTER22_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER22_HPMCOUNTER22_SW_TYPE = "WARL";


// MHPMCOUNTER23 CSR Field Defines
parameter int MHPMCOUNTER23_HPMCOUNTER23_MSB = 63;
parameter int MHPMCOUNTER23_HPMCOUNTER23_LSB = 0;
parameter int MHPMCOUNTER23_HPMCOUNTER23_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER23_HPMCOUNTER23_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER23_HPMCOUNTER23_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER23_HPMCOUNTER23_SW_TYPE = "WARL";


// MHPMCOUNTER24 CSR Field Defines
parameter int MHPMCOUNTER24_HPMCOUNTER24_MSB = 63;
parameter int MHPMCOUNTER24_HPMCOUNTER24_LSB = 0;
parameter int MHPMCOUNTER24_HPMCOUNTER24_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER24_HPMCOUNTER24_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER24_HPMCOUNTER24_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER24_HPMCOUNTER24_SW_TYPE = "WARL";


// MHPMCOUNTER25 CSR Field Defines
parameter int MHPMCOUNTER25_HPMCOUNTER25_MSB = 63;
parameter int MHPMCOUNTER25_HPMCOUNTER25_LSB = 0;
parameter int MHPMCOUNTER25_HPMCOUNTER25_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER25_HPMCOUNTER25_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER25_HPMCOUNTER25_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER25_HPMCOUNTER25_SW_TYPE = "WARL";


// MHPMCOUNTER26 CSR Field Defines
parameter int MHPMCOUNTER26_HPMCOUNTER26_MSB = 63;
parameter int MHPMCOUNTER26_HPMCOUNTER26_LSB = 0;
parameter int MHPMCOUNTER26_HPMCOUNTER26_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER26_HPMCOUNTER26_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER26_HPMCOUNTER26_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER26_HPMCOUNTER26_SW_TYPE = "WARL";


// MHPMCOUNTER27 CSR Field Defines
parameter int MHPMCOUNTER27_HPMCOUNTER27_MSB = 63;
parameter int MHPMCOUNTER27_HPMCOUNTER27_LSB = 0;
parameter int MHPMCOUNTER27_HPMCOUNTER27_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER27_HPMCOUNTER27_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER27_HPMCOUNTER27_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER27_HPMCOUNTER27_SW_TYPE = "WARL";


// MHPMCOUNTER28 CSR Field Defines
parameter int MHPMCOUNTER28_HPMCOUNTER28_MSB = 63;
parameter int MHPMCOUNTER28_HPMCOUNTER28_LSB = 0;
parameter int MHPMCOUNTER28_HPMCOUNTER28_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER28_HPMCOUNTER28_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER28_HPMCOUNTER28_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER28_HPMCOUNTER28_SW_TYPE = "WARL";


// MHPMCOUNTER29 CSR Field Defines
parameter int MHPMCOUNTER29_HPMCOUNTER29_MSB = 63;
parameter int MHPMCOUNTER29_HPMCOUNTER29_LSB = 0;
parameter int MHPMCOUNTER29_HPMCOUNTER29_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER29_HPMCOUNTER29_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER29_HPMCOUNTER29_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER29_HPMCOUNTER29_SW_TYPE = "WARL";


// MHPMCOUNTER30 CSR Field Defines
parameter int MHPMCOUNTER30_HPMCOUNTER30_MSB = 63;
parameter int MHPMCOUNTER30_HPMCOUNTER30_LSB = 0;
parameter int MHPMCOUNTER30_HPMCOUNTER30_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER30_HPMCOUNTER30_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER30_HPMCOUNTER30_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER30_HPMCOUNTER30_SW_TYPE = "WARL";


// MHPMCOUNTER31 CSR Field Defines
parameter int MHPMCOUNTER31_HPMCOUNTER31_MSB = 63;
parameter int MHPMCOUNTER31_HPMCOUNTER31_LSB = 0;
parameter int MHPMCOUNTER31_HPMCOUNTER31_WIDTH = 64;
parameter logic [63:0] MHPMCOUNTER31_HPMCOUNTER31_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMCOUNTER31_HPMCOUNTER31_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMCOUNTER31_HPMCOUNTER31_SW_TYPE = "WARL";


// MHPMEVENT3 CSR Field Defines
parameter int MHPMEVENT3_OF_MSB = 63;
parameter int MHPMEVENT3_OF_LSB = 63;
parameter int MHPMEVENT3_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT3_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT3_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT3_MINH_MSB = 62;
parameter int MHPMEVENT3_MINH_LSB = 62;
parameter int MHPMEVENT3_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT3_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT3_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT3_SINH_MSB = 61;
parameter int MHPMEVENT3_SINH_LSB = 61;
parameter int MHPMEVENT3_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT3_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT3_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT3_UINH_MSB = 60;
parameter int MHPMEVENT3_UINH_LSB = 60;
parameter int MHPMEVENT3_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT3_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT3_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT3_VSINH_MSB = 59;
parameter int MHPMEVENT3_VSINH_LSB = 59;
parameter int MHPMEVENT3_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT3_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT3_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT3_VUINH_MSB = 58;
parameter int MHPMEVENT3_VUINH_LSB = 58;
parameter int MHPMEVENT3_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT3_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT3_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT3_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT3_RESERVED_MSB = 57;
parameter int MHPMEVENT3_RESERVED_LSB = 56;
parameter int MHPMEVENT3_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT3_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT3_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT3_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT3_MHPMEVENT3_MSB = 55;
parameter int MHPMEVENT3_MHPMEVENT3_LSB = 0;
parameter int MHPMEVENT3_MHPMEVENT3_WIDTH = 56;
parameter logic [55:0] MHPMEVENT3_MHPMEVENT3_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT3_MHPMEVENT3_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT3_MHPMEVENT3_SW_TYPE = "WARL";


// MHPMEVENT4 CSR Field Defines
parameter int MHPMEVENT4_OF_MSB = 63;
parameter int MHPMEVENT4_OF_LSB = 63;
parameter int MHPMEVENT4_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT4_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT4_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT4_MINH_MSB = 62;
parameter int MHPMEVENT4_MINH_LSB = 62;
parameter int MHPMEVENT4_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT4_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT4_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT4_SINH_MSB = 61;
parameter int MHPMEVENT4_SINH_LSB = 61;
parameter int MHPMEVENT4_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT4_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT4_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT4_UINH_MSB = 60;
parameter int MHPMEVENT4_UINH_LSB = 60;
parameter int MHPMEVENT4_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT4_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT4_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT4_VSINH_MSB = 59;
parameter int MHPMEVENT4_VSINH_LSB = 59;
parameter int MHPMEVENT4_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT4_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT4_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT4_VUINH_MSB = 58;
parameter int MHPMEVENT4_VUINH_LSB = 58;
parameter int MHPMEVENT4_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT4_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT4_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT4_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT4_RESERVED_MSB = 57;
parameter int MHPMEVENT4_RESERVED_LSB = 56;
parameter int MHPMEVENT4_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT4_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT4_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT4_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT4_MHPMEVENT4_MSB = 55;
parameter int MHPMEVENT4_MHPMEVENT4_LSB = 0;
parameter int MHPMEVENT4_MHPMEVENT4_WIDTH = 56;
parameter logic [55:0] MHPMEVENT4_MHPMEVENT4_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT4_MHPMEVENT4_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT4_MHPMEVENT4_SW_TYPE = "WARL";


// MHPMEVENT5 CSR Field Defines
parameter int MHPMEVENT5_OF_MSB = 63;
parameter int MHPMEVENT5_OF_LSB = 63;
parameter int MHPMEVENT5_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT5_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT5_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT5_MINH_MSB = 62;
parameter int MHPMEVENT5_MINH_LSB = 62;
parameter int MHPMEVENT5_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT5_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT5_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT5_SINH_MSB = 61;
parameter int MHPMEVENT5_SINH_LSB = 61;
parameter int MHPMEVENT5_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT5_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT5_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT5_UINH_MSB = 60;
parameter int MHPMEVENT5_UINH_LSB = 60;
parameter int MHPMEVENT5_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT5_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT5_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT5_VSINH_MSB = 59;
parameter int MHPMEVENT5_VSINH_LSB = 59;
parameter int MHPMEVENT5_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT5_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT5_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT5_VUINH_MSB = 58;
parameter int MHPMEVENT5_VUINH_LSB = 58;
parameter int MHPMEVENT5_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT5_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT5_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT5_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT5_RESERVED_MSB = 57;
parameter int MHPMEVENT5_RESERVED_LSB = 56;
parameter int MHPMEVENT5_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT5_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT5_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT5_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT5_MHPMEVENT5_MSB = 55;
parameter int MHPMEVENT5_MHPMEVENT5_LSB = 0;
parameter int MHPMEVENT5_MHPMEVENT5_WIDTH = 56;
parameter logic [55:0] MHPMEVENT5_MHPMEVENT5_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT5_MHPMEVENT5_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT5_MHPMEVENT5_SW_TYPE = "WARL";


// MHPMEVENT6 CSR Field Defines
parameter int MHPMEVENT6_OF_MSB = 63;
parameter int MHPMEVENT6_OF_LSB = 63;
parameter int MHPMEVENT6_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT6_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT6_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT6_MINH_MSB = 62;
parameter int MHPMEVENT6_MINH_LSB = 62;
parameter int MHPMEVENT6_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT6_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT6_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT6_SINH_MSB = 61;
parameter int MHPMEVENT6_SINH_LSB = 61;
parameter int MHPMEVENT6_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT6_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT6_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT6_UINH_MSB = 60;
parameter int MHPMEVENT6_UINH_LSB = 60;
parameter int MHPMEVENT6_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT6_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT6_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT6_VSINH_MSB = 59;
parameter int MHPMEVENT6_VSINH_LSB = 59;
parameter int MHPMEVENT6_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT6_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT6_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT6_VUINH_MSB = 58;
parameter int MHPMEVENT6_VUINH_LSB = 58;
parameter int MHPMEVENT6_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT6_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT6_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT6_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT6_RESERVED_MSB = 57;
parameter int MHPMEVENT6_RESERVED_LSB = 56;
parameter int MHPMEVENT6_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT6_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT6_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT6_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT6_MHPMEVENT6_MSB = 55;
parameter int MHPMEVENT6_MHPMEVENT6_LSB = 0;
parameter int MHPMEVENT6_MHPMEVENT6_WIDTH = 56;
parameter logic [55:0] MHPMEVENT6_MHPMEVENT6_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT6_MHPMEVENT6_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT6_MHPMEVENT6_SW_TYPE = "WARL";


// MHPMEVENT7 CSR Field Defines
parameter int MHPMEVENT7_OF_MSB = 63;
parameter int MHPMEVENT7_OF_LSB = 63;
parameter int MHPMEVENT7_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT7_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT7_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT7_MINH_MSB = 62;
parameter int MHPMEVENT7_MINH_LSB = 62;
parameter int MHPMEVENT7_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT7_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT7_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT7_SINH_MSB = 61;
parameter int MHPMEVENT7_SINH_LSB = 61;
parameter int MHPMEVENT7_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT7_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT7_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT7_UINH_MSB = 60;
parameter int MHPMEVENT7_UINH_LSB = 60;
parameter int MHPMEVENT7_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT7_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT7_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT7_VSINH_MSB = 59;
parameter int MHPMEVENT7_VSINH_LSB = 59;
parameter int MHPMEVENT7_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT7_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT7_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT7_VUINH_MSB = 58;
parameter int MHPMEVENT7_VUINH_LSB = 58;
parameter int MHPMEVENT7_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT7_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT7_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT7_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT7_RESERVED_MSB = 57;
parameter int MHPMEVENT7_RESERVED_LSB = 56;
parameter int MHPMEVENT7_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT7_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT7_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT7_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT7_MHPMEVENT7_MSB = 55;
parameter int MHPMEVENT7_MHPMEVENT7_LSB = 0;
parameter int MHPMEVENT7_MHPMEVENT7_WIDTH = 56;
parameter logic [55:0] MHPMEVENT7_MHPMEVENT7_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT7_MHPMEVENT7_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT7_MHPMEVENT7_SW_TYPE = "WARL";


// MHPMEVENT8 CSR Field Defines
parameter int MHPMEVENT8_OF_MSB = 63;
parameter int MHPMEVENT8_OF_LSB = 63;
parameter int MHPMEVENT8_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT8_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT8_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT8_MINH_MSB = 62;
parameter int MHPMEVENT8_MINH_LSB = 62;
parameter int MHPMEVENT8_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT8_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT8_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT8_SINH_MSB = 61;
parameter int MHPMEVENT8_SINH_LSB = 61;
parameter int MHPMEVENT8_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT8_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT8_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT8_UINH_MSB = 60;
parameter int MHPMEVENT8_UINH_LSB = 60;
parameter int MHPMEVENT8_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT8_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT8_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT8_VSINH_MSB = 59;
parameter int MHPMEVENT8_VSINH_LSB = 59;
parameter int MHPMEVENT8_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT8_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT8_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT8_VUINH_MSB = 58;
parameter int MHPMEVENT8_VUINH_LSB = 58;
parameter int MHPMEVENT8_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT8_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT8_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT8_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT8_RESERVED_MSB = 57;
parameter int MHPMEVENT8_RESERVED_LSB = 56;
parameter int MHPMEVENT8_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT8_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT8_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT8_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT8_MHPMEVENT8_MSB = 55;
parameter int MHPMEVENT8_MHPMEVENT8_LSB = 0;
parameter int MHPMEVENT8_MHPMEVENT8_WIDTH = 56;
parameter logic [55:0] MHPMEVENT8_MHPMEVENT8_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT8_MHPMEVENT8_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT8_MHPMEVENT8_SW_TYPE = "WARL";


// MHPMEVENT9 CSR Field Defines
parameter int MHPMEVENT9_OF_MSB = 63;
parameter int MHPMEVENT9_OF_LSB = 63;
parameter int MHPMEVENT9_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT9_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT9_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT9_MINH_MSB = 62;
parameter int MHPMEVENT9_MINH_LSB = 62;
parameter int MHPMEVENT9_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT9_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT9_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT9_SINH_MSB = 61;
parameter int MHPMEVENT9_SINH_LSB = 61;
parameter int MHPMEVENT9_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT9_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT9_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT9_UINH_MSB = 60;
parameter int MHPMEVENT9_UINH_LSB = 60;
parameter int MHPMEVENT9_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT9_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT9_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT9_VSINH_MSB = 59;
parameter int MHPMEVENT9_VSINH_LSB = 59;
parameter int MHPMEVENT9_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT9_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT9_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT9_VUINH_MSB = 58;
parameter int MHPMEVENT9_VUINH_LSB = 58;
parameter int MHPMEVENT9_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT9_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT9_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT9_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT9_RESERVED_MSB = 57;
parameter int MHPMEVENT9_RESERVED_LSB = 56;
parameter int MHPMEVENT9_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT9_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT9_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT9_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT9_MHPMEVENT9_MSB = 55;
parameter int MHPMEVENT9_MHPMEVENT9_LSB = 0;
parameter int MHPMEVENT9_MHPMEVENT9_WIDTH = 56;
parameter logic [55:0] MHPMEVENT9_MHPMEVENT9_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT9_MHPMEVENT9_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT9_MHPMEVENT9_SW_TYPE = "WARL";


// MHPMEVENT10 CSR Field Defines
parameter int MHPMEVENT10_OF_MSB = 63;
parameter int MHPMEVENT10_OF_LSB = 63;
parameter int MHPMEVENT10_OF_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_OF_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MHPMEVENT10_OF_MASK = 64'h8000000000000000;
parameter string MHPMEVENT10_OF_SW_TYPE = "WARL";

parameter int MHPMEVENT10_MINH_MSB = 62;
parameter int MHPMEVENT10_MINH_LSB = 62;
parameter int MHPMEVENT10_MINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_MINH_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MHPMEVENT10_MINH_MASK = 64'h4000000000000000;
parameter string MHPMEVENT10_MINH_SW_TYPE = "WARL";

parameter int MHPMEVENT10_SINH_MSB = 61;
parameter int MHPMEVENT10_SINH_LSB = 61;
parameter int MHPMEVENT10_SINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_SINH_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MHPMEVENT10_SINH_MASK = 64'h2000000000000000;
parameter string MHPMEVENT10_SINH_SW_TYPE = "WARL";

parameter int MHPMEVENT10_UINH_MSB = 60;
parameter int MHPMEVENT10_UINH_LSB = 60;
parameter int MHPMEVENT10_UINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_UINH_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MHPMEVENT10_UINH_MASK = 64'h1000000000000000;
parameter string MHPMEVENT10_UINH_SW_TYPE = "WARL";

parameter int MHPMEVENT10_VSINH_MSB = 59;
parameter int MHPMEVENT10_VSINH_LSB = 59;
parameter int MHPMEVENT10_VSINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_VSINH_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MHPMEVENT10_VSINH_MASK = 64'h0800000000000000;
parameter string MHPMEVENT10_VSINH_SW_TYPE = "WARL";

parameter int MHPMEVENT10_VUINH_MSB = 58;
parameter int MHPMEVENT10_VUINH_LSB = 58;
parameter int MHPMEVENT10_VUINH_WIDTH = 1;
parameter logic [0:0] MHPMEVENT10_VUINH_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MHPMEVENT10_VUINH_MASK = 64'h0400000000000000;
parameter string MHPMEVENT10_VUINH_SW_TYPE = "WARL";

parameter int MHPMEVENT10_RESERVED_MSB = 57;
parameter int MHPMEVENT10_RESERVED_LSB = 56;
parameter int MHPMEVENT10_RESERVED_WIDTH = 2;
parameter logic [1:0] MHPMEVENT10_RESERVED_RESET = 64'h0000000000000000[57:56];
parameter logic [63:0] MHPMEVENT10_RESERVED_MASK = 64'h0300000000000000;
parameter string MHPMEVENT10_RESERVED_SW_TYPE = "WARL";

parameter int MHPMEVENT10_MHPMEVENT10_MSB = 55;
parameter int MHPMEVENT10_MHPMEVENT10_LSB = 0;
parameter int MHPMEVENT10_MHPMEVENT10_WIDTH = 56;
parameter logic [55:0] MHPMEVENT10_MHPMEVENT10_RESET = 64'h0000000000000000[55:0];
parameter logic [63:0] MHPMEVENT10_MHPMEVENT10_MASK = 64'h00FFFFFFFFFFFFFF;
parameter string MHPMEVENT10_MHPMEVENT10_SW_TYPE = "WARL";


// MHPMEVENT11 CSR Field Defines
parameter int MHPMEVENT11_MHPMEVENT11_MSB = 63;
parameter int MHPMEVENT11_MHPMEVENT11_LSB = 0;
parameter int MHPMEVENT11_MHPMEVENT11_WIDTH = 64;
parameter logic [63:0] MHPMEVENT11_MHPMEVENT11_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT11_MHPMEVENT11_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT11_MHPMEVENT11_SW_TYPE = "WARL";


// MHPMEVENT12 CSR Field Defines
parameter int MHPMEVENT12_MHPMEVENT12_MSB = 63;
parameter int MHPMEVENT12_MHPMEVENT12_LSB = 0;
parameter int MHPMEVENT12_MHPMEVENT12_WIDTH = 64;
parameter logic [63:0] MHPMEVENT12_MHPMEVENT12_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT12_MHPMEVENT12_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT12_MHPMEVENT12_SW_TYPE = "WARL";


// MHPMEVENT13 CSR Field Defines
parameter int MHPMEVENT13_MHPMEVENT13_MSB = 63;
parameter int MHPMEVENT13_MHPMEVENT13_LSB = 0;
parameter int MHPMEVENT13_MHPMEVENT13_WIDTH = 64;
parameter logic [63:0] MHPMEVENT13_MHPMEVENT13_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT13_MHPMEVENT13_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT13_MHPMEVENT13_SW_TYPE = "WARL";


// MHPMEVENT14 CSR Field Defines
parameter int MHPMEVENT14_MHPMEVENT14_MSB = 63;
parameter int MHPMEVENT14_MHPMEVENT14_LSB = 0;
parameter int MHPMEVENT14_MHPMEVENT14_WIDTH = 64;
parameter logic [63:0] MHPMEVENT14_MHPMEVENT14_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT14_MHPMEVENT14_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT14_MHPMEVENT14_SW_TYPE = "WARL";


// MHPMEVENT15 CSR Field Defines
parameter int MHPMEVENT15_MHPMEVENT15_MSB = 63;
parameter int MHPMEVENT15_MHPMEVENT15_LSB = 0;
parameter int MHPMEVENT15_MHPMEVENT15_WIDTH = 64;
parameter logic [63:0] MHPMEVENT15_MHPMEVENT15_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT15_MHPMEVENT15_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT15_MHPMEVENT15_SW_TYPE = "WARL";


// MHPMEVENT16 CSR Field Defines
parameter int MHPMEVENT16_MHPMEVENT16_MSB = 63;
parameter int MHPMEVENT16_MHPMEVENT16_LSB = 0;
parameter int MHPMEVENT16_MHPMEVENT16_WIDTH = 64;
parameter logic [63:0] MHPMEVENT16_MHPMEVENT16_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT16_MHPMEVENT16_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT16_MHPMEVENT16_SW_TYPE = "WARL";


// MHPMEVENT17 CSR Field Defines
parameter int MHPMEVENT17_MHPMEVENT17_MSB = 63;
parameter int MHPMEVENT17_MHPMEVENT17_LSB = 0;
parameter int MHPMEVENT17_MHPMEVENT17_WIDTH = 64;
parameter logic [63:0] MHPMEVENT17_MHPMEVENT17_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT17_MHPMEVENT17_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT17_MHPMEVENT17_SW_TYPE = "WARL";


// MHPMEVENT18 CSR Field Defines
parameter int MHPMEVENT18_MHPMEVENT18_MSB = 63;
parameter int MHPMEVENT18_MHPMEVENT18_LSB = 0;
parameter int MHPMEVENT18_MHPMEVENT18_WIDTH = 64;
parameter logic [63:0] MHPMEVENT18_MHPMEVENT18_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT18_MHPMEVENT18_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT18_MHPMEVENT18_SW_TYPE = "WARL";


// MHPMEVENT19 CSR Field Defines
parameter int MHPMEVENT19_MHPMEVENT19_MSB = 63;
parameter int MHPMEVENT19_MHPMEVENT19_LSB = 0;
parameter int MHPMEVENT19_MHPMEVENT19_WIDTH = 64;
parameter logic [63:0] MHPMEVENT19_MHPMEVENT19_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT19_MHPMEVENT19_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT19_MHPMEVENT19_SW_TYPE = "WARL";


// MHPMEVENT20 CSR Field Defines
parameter int MHPMEVENT20_MHPMEVENT20_MSB = 63;
parameter int MHPMEVENT20_MHPMEVENT20_LSB = 0;
parameter int MHPMEVENT20_MHPMEVENT20_WIDTH = 64;
parameter logic [63:0] MHPMEVENT20_MHPMEVENT20_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT20_MHPMEVENT20_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT20_MHPMEVENT20_SW_TYPE = "WARL";


// MHPMEVENT21 CSR Field Defines
parameter int MHPMEVENT21_MHPMEVENT21_MSB = 63;
parameter int MHPMEVENT21_MHPMEVENT21_LSB = 0;
parameter int MHPMEVENT21_MHPMEVENT21_WIDTH = 64;
parameter logic [63:0] MHPMEVENT21_MHPMEVENT21_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT21_MHPMEVENT21_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT21_MHPMEVENT21_SW_TYPE = "WARL";


// MHPMEVENT22 CSR Field Defines
parameter int MHPMEVENT22_MHPMEVENT22_MSB = 63;
parameter int MHPMEVENT22_MHPMEVENT22_LSB = 0;
parameter int MHPMEVENT22_MHPMEVENT22_WIDTH = 64;
parameter logic [63:0] MHPMEVENT22_MHPMEVENT22_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT22_MHPMEVENT22_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT22_MHPMEVENT22_SW_TYPE = "WARL";


// MHPMEVENT23 CSR Field Defines
parameter int MHPMEVENT23_MHPMEVENT23_MSB = 63;
parameter int MHPMEVENT23_MHPMEVENT23_LSB = 0;
parameter int MHPMEVENT23_MHPMEVENT23_WIDTH = 64;
parameter logic [63:0] MHPMEVENT23_MHPMEVENT23_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT23_MHPMEVENT23_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT23_MHPMEVENT23_SW_TYPE = "WARL";


// MHPMEVENT24 CSR Field Defines
parameter int MHPMEVENT24_MHPMEVENT24_MSB = 63;
parameter int MHPMEVENT24_MHPMEVENT24_LSB = 0;
parameter int MHPMEVENT24_MHPMEVENT24_WIDTH = 64;
parameter logic [63:0] MHPMEVENT24_MHPMEVENT24_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT24_MHPMEVENT24_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT24_MHPMEVENT24_SW_TYPE = "WARL";


// MHPMEVENT25 CSR Field Defines
parameter int MHPMEVENT25_MHPMEVENT25_MSB = 63;
parameter int MHPMEVENT25_MHPMEVENT25_LSB = 0;
parameter int MHPMEVENT25_MHPMEVENT25_WIDTH = 64;
parameter logic [63:0] MHPMEVENT25_MHPMEVENT25_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT25_MHPMEVENT25_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT25_MHPMEVENT25_SW_TYPE = "WARL";


// MHPMEVENT26 CSR Field Defines
parameter int MHPMEVENT26_MHPMEVENT26_MSB = 63;
parameter int MHPMEVENT26_MHPMEVENT26_LSB = 0;
parameter int MHPMEVENT26_MHPMEVENT26_WIDTH = 64;
parameter logic [63:0] MHPMEVENT26_MHPMEVENT26_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT26_MHPMEVENT26_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT26_MHPMEVENT26_SW_TYPE = "WARL";


// MHPMEVENT27 CSR Field Defines
parameter int MHPMEVENT27_MHPMEVENT27_MSB = 63;
parameter int MHPMEVENT27_MHPMEVENT27_LSB = 0;
parameter int MHPMEVENT27_MHPMEVENT27_WIDTH = 64;
parameter logic [63:0] MHPMEVENT27_MHPMEVENT27_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT27_MHPMEVENT27_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT27_MHPMEVENT27_SW_TYPE = "WARL";


// MHPMEVENT28 CSR Field Defines
parameter int MHPMEVENT28_MHPMEVENT28_MSB = 63;
parameter int MHPMEVENT28_MHPMEVENT28_LSB = 0;
parameter int MHPMEVENT28_MHPMEVENT28_WIDTH = 64;
parameter logic [63:0] MHPMEVENT28_MHPMEVENT28_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT28_MHPMEVENT28_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT28_MHPMEVENT28_SW_TYPE = "WARL";


// MHPMEVENT29 CSR Field Defines
parameter int MHPMEVENT29_MHPMEVENT29_MSB = 63;
parameter int MHPMEVENT29_MHPMEVENT29_LSB = 0;
parameter int MHPMEVENT29_MHPMEVENT29_WIDTH = 64;
parameter logic [63:0] MHPMEVENT29_MHPMEVENT29_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT29_MHPMEVENT29_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT29_MHPMEVENT29_SW_TYPE = "WARL";


// MHPMEVENT30 CSR Field Defines
parameter int MHPMEVENT30_MHPMEVENT30_MSB = 63;
parameter int MHPMEVENT30_MHPMEVENT30_LSB = 0;
parameter int MHPMEVENT30_MHPMEVENT30_WIDTH = 64;
parameter logic [63:0] MHPMEVENT30_MHPMEVENT30_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT30_MHPMEVENT30_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT30_MHPMEVENT30_SW_TYPE = "WARL";


// MHPMEVENT31 CSR Field Defines
parameter int MHPMEVENT31_MHPMEVENT31_MSB = 63;
parameter int MHPMEVENT31_MHPMEVENT31_LSB = 0;
parameter int MHPMEVENT31_MHPMEVENT31_WIDTH = 64;
parameter logic [63:0] MHPMEVENT31_MHPMEVENT31_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MHPMEVENT31_MHPMEVENT31_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MHPMEVENT31_MHPMEVENT31_SW_TYPE = "WARL";


// MCOUNTEREN CSR Field Defines
parameter int MCOUNTEREN_HPM31_MSB = 31;
parameter int MCOUNTEREN_HPM31_LSB = 31;
parameter int MCOUNTEREN_HPM31_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM31_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] MCOUNTEREN_HPM31_MASK = 64'h0000000080000000;
parameter string MCOUNTEREN_HPM31_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM30_MSB = 30;
parameter int MCOUNTEREN_HPM30_LSB = 30;
parameter int MCOUNTEREN_HPM30_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM30_RESET = 64'h0000000000000000[30:30];
parameter logic [63:0] MCOUNTEREN_HPM30_MASK = 64'h0000000040000000;
parameter string MCOUNTEREN_HPM30_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM29_MSB = 29;
parameter int MCOUNTEREN_HPM29_LSB = 29;
parameter int MCOUNTEREN_HPM29_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM29_RESET = 64'h0000000000000000[29:29];
parameter logic [63:0] MCOUNTEREN_HPM29_MASK = 64'h0000000020000000;
parameter string MCOUNTEREN_HPM29_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM28_MSB = 28;
parameter int MCOUNTEREN_HPM28_LSB = 28;
parameter int MCOUNTEREN_HPM28_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM28_RESET = 64'h0000000000000000[28:28];
parameter logic [63:0] MCOUNTEREN_HPM28_MASK = 64'h0000000010000000;
parameter string MCOUNTEREN_HPM28_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM27_MSB = 27;
parameter int MCOUNTEREN_HPM27_LSB = 27;
parameter int MCOUNTEREN_HPM27_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM27_RESET = 64'h0000000000000000[27:27];
parameter logic [63:0] MCOUNTEREN_HPM27_MASK = 64'h0000000008000000;
parameter string MCOUNTEREN_HPM27_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM26_MSB = 26;
parameter int MCOUNTEREN_HPM26_LSB = 26;
parameter int MCOUNTEREN_HPM26_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM26_RESET = 64'h0000000000000000[26:26];
parameter logic [63:0] MCOUNTEREN_HPM26_MASK = 64'h0000000004000000;
parameter string MCOUNTEREN_HPM26_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM25_MSB = 25;
parameter int MCOUNTEREN_HPM25_LSB = 25;
parameter int MCOUNTEREN_HPM25_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM25_RESET = 64'h0000000000000000[25:25];
parameter logic [63:0] MCOUNTEREN_HPM25_MASK = 64'h0000000002000000;
parameter string MCOUNTEREN_HPM25_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM24_MSB = 24;
parameter int MCOUNTEREN_HPM24_LSB = 24;
parameter int MCOUNTEREN_HPM24_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM24_RESET = 64'h0000000000000000[24:24];
parameter logic [63:0] MCOUNTEREN_HPM24_MASK = 64'h0000000001000000;
parameter string MCOUNTEREN_HPM24_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM23_MSB = 23;
parameter int MCOUNTEREN_HPM23_LSB = 23;
parameter int MCOUNTEREN_HPM23_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM23_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] MCOUNTEREN_HPM23_MASK = 64'h0000000000800000;
parameter string MCOUNTEREN_HPM23_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM22_MSB = 22;
parameter int MCOUNTEREN_HPM22_LSB = 22;
parameter int MCOUNTEREN_HPM22_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM22_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] MCOUNTEREN_HPM22_MASK = 64'h0000000000400000;
parameter string MCOUNTEREN_HPM22_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM21_MSB = 21;
parameter int MCOUNTEREN_HPM21_LSB = 21;
parameter int MCOUNTEREN_HPM21_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM21_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] MCOUNTEREN_HPM21_MASK = 64'h0000000000200000;
parameter string MCOUNTEREN_HPM21_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM20_MSB = 20;
parameter int MCOUNTEREN_HPM20_LSB = 20;
parameter int MCOUNTEREN_HPM20_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM20_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] MCOUNTEREN_HPM20_MASK = 64'h0000000000100000;
parameter string MCOUNTEREN_HPM20_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM19_MSB = 19;
parameter int MCOUNTEREN_HPM19_LSB = 19;
parameter int MCOUNTEREN_HPM19_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM19_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] MCOUNTEREN_HPM19_MASK = 64'h0000000000080000;
parameter string MCOUNTEREN_HPM19_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM18_MSB = 18;
parameter int MCOUNTEREN_HPM18_LSB = 18;
parameter int MCOUNTEREN_HPM18_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM18_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] MCOUNTEREN_HPM18_MASK = 64'h0000000000040000;
parameter string MCOUNTEREN_HPM18_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM17_MSB = 17;
parameter int MCOUNTEREN_HPM17_LSB = 17;
parameter int MCOUNTEREN_HPM17_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM17_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] MCOUNTEREN_HPM17_MASK = 64'h0000000000020000;
parameter string MCOUNTEREN_HPM17_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM16_MSB = 16;
parameter int MCOUNTEREN_HPM16_LSB = 16;
parameter int MCOUNTEREN_HPM16_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM16_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] MCOUNTEREN_HPM16_MASK = 64'h0000000000010000;
parameter string MCOUNTEREN_HPM16_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM15_MSB = 15;
parameter int MCOUNTEREN_HPM15_LSB = 15;
parameter int MCOUNTEREN_HPM15_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM15_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] MCOUNTEREN_HPM15_MASK = 64'h0000000000008000;
parameter string MCOUNTEREN_HPM15_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM14_MSB = 14;
parameter int MCOUNTEREN_HPM14_LSB = 14;
parameter int MCOUNTEREN_HPM14_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM14_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] MCOUNTEREN_HPM14_MASK = 64'h0000000000004000;
parameter string MCOUNTEREN_HPM14_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM13_MSB = 13;
parameter int MCOUNTEREN_HPM13_LSB = 13;
parameter int MCOUNTEREN_HPM13_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM13_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MCOUNTEREN_HPM13_MASK = 64'h0000000000002000;
parameter string MCOUNTEREN_HPM13_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM12_MSB = 12;
parameter int MCOUNTEREN_HPM12_LSB = 12;
parameter int MCOUNTEREN_HPM12_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM12_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] MCOUNTEREN_HPM12_MASK = 64'h0000000000001000;
parameter string MCOUNTEREN_HPM12_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM11_MSB = 11;
parameter int MCOUNTEREN_HPM11_LSB = 11;
parameter int MCOUNTEREN_HPM11_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM11_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MCOUNTEREN_HPM11_MASK = 64'h0000000000000800;
parameter string MCOUNTEREN_HPM11_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM10_MSB = 10;
parameter int MCOUNTEREN_HPM10_LSB = 10;
parameter int MCOUNTEREN_HPM10_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM10_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MCOUNTEREN_HPM10_MASK = 64'h0000000000000400;
parameter string MCOUNTEREN_HPM10_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM9_MSB = 9;
parameter int MCOUNTEREN_HPM9_LSB = 9;
parameter int MCOUNTEREN_HPM9_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM9_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MCOUNTEREN_HPM9_MASK = 64'h0000000000000200;
parameter string MCOUNTEREN_HPM9_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM8_MSB = 8;
parameter int MCOUNTEREN_HPM8_LSB = 8;
parameter int MCOUNTEREN_HPM8_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM8_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] MCOUNTEREN_HPM8_MASK = 64'h0000000000000100;
parameter string MCOUNTEREN_HPM8_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM7_MSB = 7;
parameter int MCOUNTEREN_HPM7_LSB = 7;
parameter int MCOUNTEREN_HPM7_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM7_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MCOUNTEREN_HPM7_MASK = 64'h0000000000000080;
parameter string MCOUNTEREN_HPM7_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM6_MSB = 6;
parameter int MCOUNTEREN_HPM6_LSB = 6;
parameter int MCOUNTEREN_HPM6_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM6_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MCOUNTEREN_HPM6_MASK = 64'h0000000000000040;
parameter string MCOUNTEREN_HPM6_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM5_MSB = 5;
parameter int MCOUNTEREN_HPM5_LSB = 5;
parameter int MCOUNTEREN_HPM5_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM5_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MCOUNTEREN_HPM5_MASK = 64'h0000000000000020;
parameter string MCOUNTEREN_HPM5_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM4_MSB = 4;
parameter int MCOUNTEREN_HPM4_LSB = 4;
parameter int MCOUNTEREN_HPM4_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM4_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] MCOUNTEREN_HPM4_MASK = 64'h0000000000000010;
parameter string MCOUNTEREN_HPM4_SW_TYPE = "WARL";

parameter int MCOUNTEREN_HPM3_MSB = 3;
parameter int MCOUNTEREN_HPM3_LSB = 3;
parameter int MCOUNTEREN_HPM3_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_HPM3_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MCOUNTEREN_HPM3_MASK = 64'h0000000000000008;
parameter string MCOUNTEREN_HPM3_SW_TYPE = "WARL";

parameter int MCOUNTEREN_IR_MSB = 2;
parameter int MCOUNTEREN_IR_LSB = 2;
parameter int MCOUNTEREN_IR_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_IR_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MCOUNTEREN_IR_MASK = 64'h0000000000000004;
parameter string MCOUNTEREN_IR_SW_TYPE = "WARL";

parameter int MCOUNTEREN_TM_MSB = 1;
parameter int MCOUNTEREN_TM_LSB = 1;
parameter int MCOUNTEREN_TM_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_TM_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MCOUNTEREN_TM_MASK = 64'h0000000000000002;
parameter string MCOUNTEREN_TM_SW_TYPE = "WARL";

parameter int MCOUNTEREN_CY_MSB = 0;
parameter int MCOUNTEREN_CY_LSB = 0;
parameter int MCOUNTEREN_CY_WIDTH = 1;
parameter logic [0:0] MCOUNTEREN_CY_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MCOUNTEREN_CY_MASK = 64'h0000000000000001;
parameter string MCOUNTEREN_CY_SW_TYPE = "WARL";


// MCOUNTINHIBIT CSR Field Defines
parameter int MCOUNTINHIBIT_HPM31_MSB = 31;
parameter int MCOUNTINHIBIT_HPM31_LSB = 31;
parameter int MCOUNTINHIBIT_HPM31_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM31_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] MCOUNTINHIBIT_HPM31_MASK = 64'h0000000080000000;
parameter string MCOUNTINHIBIT_HPM31_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM30_MSB = 30;
parameter int MCOUNTINHIBIT_HPM30_LSB = 30;
parameter int MCOUNTINHIBIT_HPM30_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM30_RESET = 64'h0000000000000000[30:30];
parameter logic [63:0] MCOUNTINHIBIT_HPM30_MASK = 64'h0000000040000000;
parameter string MCOUNTINHIBIT_HPM30_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM29_MSB = 29;
parameter int MCOUNTINHIBIT_HPM29_LSB = 29;
parameter int MCOUNTINHIBIT_HPM29_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM29_RESET = 64'h0000000000000000[29:29];
parameter logic [63:0] MCOUNTINHIBIT_HPM29_MASK = 64'h0000000020000000;
parameter string MCOUNTINHIBIT_HPM29_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM28_MSB = 28;
parameter int MCOUNTINHIBIT_HPM28_LSB = 28;
parameter int MCOUNTINHIBIT_HPM28_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM28_RESET = 64'h0000000000000000[28:28];
parameter logic [63:0] MCOUNTINHIBIT_HPM28_MASK = 64'h0000000010000000;
parameter string MCOUNTINHIBIT_HPM28_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM27_MSB = 27;
parameter int MCOUNTINHIBIT_HPM27_LSB = 27;
parameter int MCOUNTINHIBIT_HPM27_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM27_RESET = 64'h0000000000000000[27:27];
parameter logic [63:0] MCOUNTINHIBIT_HPM27_MASK = 64'h0000000008000000;
parameter string MCOUNTINHIBIT_HPM27_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM26_MSB = 26;
parameter int MCOUNTINHIBIT_HPM26_LSB = 26;
parameter int MCOUNTINHIBIT_HPM26_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM26_RESET = 64'h0000000000000000[26:26];
parameter logic [63:0] MCOUNTINHIBIT_HPM26_MASK = 64'h0000000004000000;
parameter string MCOUNTINHIBIT_HPM26_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM25_MSB = 25;
parameter int MCOUNTINHIBIT_HPM25_LSB = 25;
parameter int MCOUNTINHIBIT_HPM25_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM25_RESET = 64'h0000000000000000[25:25];
parameter logic [63:0] MCOUNTINHIBIT_HPM25_MASK = 64'h0000000002000000;
parameter string MCOUNTINHIBIT_HPM25_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM24_MSB = 24;
parameter int MCOUNTINHIBIT_HPM24_LSB = 24;
parameter int MCOUNTINHIBIT_HPM24_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM24_RESET = 64'h0000000000000000[24:24];
parameter logic [63:0] MCOUNTINHIBIT_HPM24_MASK = 64'h0000000001000000;
parameter string MCOUNTINHIBIT_HPM24_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM23_MSB = 23;
parameter int MCOUNTINHIBIT_HPM23_LSB = 23;
parameter int MCOUNTINHIBIT_HPM23_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM23_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] MCOUNTINHIBIT_HPM23_MASK = 64'h0000000000800000;
parameter string MCOUNTINHIBIT_HPM23_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM22_MSB = 22;
parameter int MCOUNTINHIBIT_HPM22_LSB = 22;
parameter int MCOUNTINHIBIT_HPM22_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM22_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] MCOUNTINHIBIT_HPM22_MASK = 64'h0000000000400000;
parameter string MCOUNTINHIBIT_HPM22_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM21_MSB = 21;
parameter int MCOUNTINHIBIT_HPM21_LSB = 21;
parameter int MCOUNTINHIBIT_HPM21_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM21_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] MCOUNTINHIBIT_HPM21_MASK = 64'h0000000000200000;
parameter string MCOUNTINHIBIT_HPM21_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM20_MSB = 20;
parameter int MCOUNTINHIBIT_HPM20_LSB = 20;
parameter int MCOUNTINHIBIT_HPM20_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM20_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] MCOUNTINHIBIT_HPM20_MASK = 64'h0000000000100000;
parameter string MCOUNTINHIBIT_HPM20_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM19_MSB = 19;
parameter int MCOUNTINHIBIT_HPM19_LSB = 19;
parameter int MCOUNTINHIBIT_HPM19_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM19_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] MCOUNTINHIBIT_HPM19_MASK = 64'h0000000000080000;
parameter string MCOUNTINHIBIT_HPM19_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM18_MSB = 18;
parameter int MCOUNTINHIBIT_HPM18_LSB = 18;
parameter int MCOUNTINHIBIT_HPM18_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM18_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] MCOUNTINHIBIT_HPM18_MASK = 64'h0000000000040000;
parameter string MCOUNTINHIBIT_HPM18_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM17_MSB = 17;
parameter int MCOUNTINHIBIT_HPM17_LSB = 17;
parameter int MCOUNTINHIBIT_HPM17_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM17_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] MCOUNTINHIBIT_HPM17_MASK = 64'h0000000000020000;
parameter string MCOUNTINHIBIT_HPM17_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM16_MSB = 16;
parameter int MCOUNTINHIBIT_HPM16_LSB = 16;
parameter int MCOUNTINHIBIT_HPM16_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM16_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] MCOUNTINHIBIT_HPM16_MASK = 64'h0000000000010000;
parameter string MCOUNTINHIBIT_HPM16_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM15_MSB = 15;
parameter int MCOUNTINHIBIT_HPM15_LSB = 15;
parameter int MCOUNTINHIBIT_HPM15_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM15_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] MCOUNTINHIBIT_HPM15_MASK = 64'h0000000000008000;
parameter string MCOUNTINHIBIT_HPM15_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM14_MSB = 14;
parameter int MCOUNTINHIBIT_HPM14_LSB = 14;
parameter int MCOUNTINHIBIT_HPM14_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM14_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] MCOUNTINHIBIT_HPM14_MASK = 64'h0000000000004000;
parameter string MCOUNTINHIBIT_HPM14_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM13_MSB = 13;
parameter int MCOUNTINHIBIT_HPM13_LSB = 13;
parameter int MCOUNTINHIBIT_HPM13_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM13_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MCOUNTINHIBIT_HPM13_MASK = 64'h0000000000002000;
parameter string MCOUNTINHIBIT_HPM13_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM12_MSB = 12;
parameter int MCOUNTINHIBIT_HPM12_LSB = 12;
parameter int MCOUNTINHIBIT_HPM12_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM12_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] MCOUNTINHIBIT_HPM12_MASK = 64'h0000000000001000;
parameter string MCOUNTINHIBIT_HPM12_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM11_MSB = 11;
parameter int MCOUNTINHIBIT_HPM11_LSB = 11;
parameter int MCOUNTINHIBIT_HPM11_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM11_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] MCOUNTINHIBIT_HPM11_MASK = 64'h0000000000000800;
parameter string MCOUNTINHIBIT_HPM11_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM10_MSB = 10;
parameter int MCOUNTINHIBIT_HPM10_LSB = 10;
parameter int MCOUNTINHIBIT_HPM10_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM10_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] MCOUNTINHIBIT_HPM10_MASK = 64'h0000000000000400;
parameter string MCOUNTINHIBIT_HPM10_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM9_MSB = 9;
parameter int MCOUNTINHIBIT_HPM9_LSB = 9;
parameter int MCOUNTINHIBIT_HPM9_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM9_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MCOUNTINHIBIT_HPM9_MASK = 64'h0000000000000200;
parameter string MCOUNTINHIBIT_HPM9_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM8_MSB = 8;
parameter int MCOUNTINHIBIT_HPM8_LSB = 8;
parameter int MCOUNTINHIBIT_HPM8_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM8_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] MCOUNTINHIBIT_HPM8_MASK = 64'h0000000000000100;
parameter string MCOUNTINHIBIT_HPM8_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM7_MSB = 7;
parameter int MCOUNTINHIBIT_HPM7_LSB = 7;
parameter int MCOUNTINHIBIT_HPM7_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM7_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MCOUNTINHIBIT_HPM7_MASK = 64'h0000000000000080;
parameter string MCOUNTINHIBIT_HPM7_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM6_MSB = 6;
parameter int MCOUNTINHIBIT_HPM6_LSB = 6;
parameter int MCOUNTINHIBIT_HPM6_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM6_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] MCOUNTINHIBIT_HPM6_MASK = 64'h0000000000000040;
parameter string MCOUNTINHIBIT_HPM6_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM5_MSB = 5;
parameter int MCOUNTINHIBIT_HPM5_LSB = 5;
parameter int MCOUNTINHIBIT_HPM5_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM5_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MCOUNTINHIBIT_HPM5_MASK = 64'h0000000000000020;
parameter string MCOUNTINHIBIT_HPM5_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM4_MSB = 4;
parameter int MCOUNTINHIBIT_HPM4_LSB = 4;
parameter int MCOUNTINHIBIT_HPM4_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM4_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] MCOUNTINHIBIT_HPM4_MASK = 64'h0000000000000010;
parameter string MCOUNTINHIBIT_HPM4_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HPM3_MSB = 3;
parameter int MCOUNTINHIBIT_HPM3_LSB = 3;
parameter int MCOUNTINHIBIT_HPM3_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HPM3_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MCOUNTINHIBIT_HPM3_MASK = 64'h0000000000000008;
parameter string MCOUNTINHIBIT_HPM3_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_IR_MSB = 2;
parameter int MCOUNTINHIBIT_IR_LSB = 2;
parameter int MCOUNTINHIBIT_IR_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_IR_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MCOUNTINHIBIT_IR_MASK = 64'h0000000000000004;
parameter string MCOUNTINHIBIT_IR_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_HARD0_MSB = 1;
parameter int MCOUNTINHIBIT_HARD0_LSB = 1;
parameter int MCOUNTINHIBIT_HARD0_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_HARD0_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MCOUNTINHIBIT_HARD0_MASK = 64'h0000000000000002;
parameter string MCOUNTINHIBIT_HARD0_SW_TYPE = "WARL";

parameter int MCOUNTINHIBIT_CY_MSB = 0;
parameter int MCOUNTINHIBIT_CY_LSB = 0;
parameter int MCOUNTINHIBIT_CY_WIDTH = 1;
parameter logic [0:0] MCOUNTINHIBIT_CY_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MCOUNTINHIBIT_CY_MASK = 64'h0000000000000001;
parameter string MCOUNTINHIBIT_CY_SW_TYPE = "WARL";


// MISELECT CSR Field Defines
parameter int MISELECT_RSVD_63_8_MSB = 63;
parameter int MISELECT_RSVD_63_8_LSB = 8;
parameter int MISELECT_RSVD_63_8_WIDTH = 56;
parameter logic [55:0] MISELECT_RSVD_63_8_RESET = 64'h0000000000000000[63:8];
parameter logic [63:0] MISELECT_RSVD_63_8_MASK = 64'hFFFFFFFFFFFFFF00;
parameter string MISELECT_RSVD_63_8_SW_TYPE = "WARL";

parameter int MISELECT_INTERRUPTS_MSB = 7;
parameter int MISELECT_INTERRUPTS_LSB = 0;
parameter int MISELECT_INTERRUPTS_WIDTH = 8;
parameter logic [7:0] MISELECT_INTERRUPTS_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] MISELECT_INTERRUPTS_MASK = 64'h00000000000000FF;
parameter string MISELECT_INTERRUPTS_SW_TYPE = "WARL";


// MIREG CSR Field Defines
parameter int MIREG_MIREG_MSB = 63;
parameter int MIREG_MIREG_LSB = 0;
parameter int MIREG_MIREG_WIDTH = 64;
parameter logic [63:0] MIREG_MIREG_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MIREG_MIREG_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MIREG_MIREG_SW_TYPE = "WARL";


// MTOPEI CSR Field Defines
parameter int MTOPEI_RSVD_63_27_MSB = 63;
parameter int MTOPEI_RSVD_63_27_LSB = 27;
parameter int MTOPEI_RSVD_63_27_WIDTH = 37;
parameter logic [36:0] MTOPEI_RSVD_63_27_RESET = 64'h0000000000000000[63:27];
parameter logic [63:0] MTOPEI_RSVD_63_27_MASK = 64'hFFFFFFFFF8000000;
parameter string MTOPEI_RSVD_63_27_SW_TYPE = "WARL";

parameter int MTOPEI_IDENTITY_MSB = 26;
parameter int MTOPEI_IDENTITY_LSB = 16;
parameter int MTOPEI_IDENTITY_WIDTH = 11;
parameter logic [10:0] MTOPEI_IDENTITY_RESET = 64'h0000000000000000[26:16];
parameter logic [63:0] MTOPEI_IDENTITY_MASK = 64'h0000000007FF0000;
parameter string MTOPEI_IDENTITY_SW_TYPE = "WARL";

parameter int MTOPEI_RSVD_15_11_MSB = 15;
parameter int MTOPEI_RSVD_15_11_LSB = 11;
parameter int MTOPEI_RSVD_15_11_WIDTH = 5;
parameter logic [4:0] MTOPEI_RSVD_15_11_RESET = 64'h0000000000000000[15:11];
parameter logic [63:0] MTOPEI_RSVD_15_11_MASK = 64'h000000000000F800;
parameter string MTOPEI_RSVD_15_11_SW_TYPE = "WARL";

parameter int MTOPEI_PRIORITY_MSB = 10;
parameter int MTOPEI_PRIORITY_LSB = 0;
parameter int MTOPEI_PRIORITY_WIDTH = 11;
parameter logic [10:0] MTOPEI_PRIORITY_RESET = 64'h0000000000000000[10:0];
parameter logic [63:0] MTOPEI_PRIORITY_MASK = 64'h00000000000007FF;
parameter string MTOPEI_PRIORITY_SW_TYPE = "WARL";


// MTOPI CSR Field Defines
parameter int MTOPI_RSVD_63_28_MSB = 63;
parameter int MTOPI_RSVD_63_28_LSB = 28;
parameter int MTOPI_RSVD_63_28_WIDTH = 36;
parameter logic [35:0] MTOPI_RSVD_63_28_RESET = 64'h0000000000000000[63:28];
parameter logic [63:0] MTOPI_RSVD_63_28_MASK = 64'hFFFFFFFFF0000000;
parameter string MTOPI_RSVD_63_28_SW_TYPE = "WARL";

parameter int MTOPI_IID_MSB = 27;
parameter int MTOPI_IID_LSB = 16;
parameter int MTOPI_IID_WIDTH = 12;
parameter logic [11:0] MTOPI_IID_RESET = 64'h0000000000000000[27:16];
parameter logic [63:0] MTOPI_IID_MASK = 64'h000000000FFF0000;
parameter string MTOPI_IID_SW_TYPE = "WARL";

parameter int MTOPI_RSVD_15_8_MSB = 15;
parameter int MTOPI_RSVD_15_8_LSB = 8;
parameter int MTOPI_RSVD_15_8_WIDTH = 8;
parameter logic [7:0] MTOPI_RSVD_15_8_RESET = 64'h0000000000000000[15:8];
parameter logic [63:0] MTOPI_RSVD_15_8_MASK = 64'h000000000000FF00;
parameter string MTOPI_RSVD_15_8_SW_TYPE = "WARL";

parameter int MTOPI_IPRIO_MSB = 7;
parameter int MTOPI_IPRIO_LSB = 0;
parameter int MTOPI_IPRIO_WIDTH = 8;
parameter logic [7:0] MTOPI_IPRIO_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] MTOPI_IPRIO_MASK = 64'h00000000000000FF;
parameter string MTOPI_IPRIO_SW_TYPE = "WARL";


// MVIEN CSR Field Defines
parameter int MVIEN_LCOFIP_MSB = 13;
parameter int MVIEN_LCOFIP_LSB = 13;
parameter int MVIEN_LCOFIP_WIDTH = 1;
parameter logic [0:0] MVIEN_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MVIEN_LCOFIP_MASK = 64'h0000000000002000;
parameter string MVIEN_LCOFIP_SW_TYPE = "WARL";

parameter int MVIEN_HARD0_2_MSB = 12;
parameter int MVIEN_HARD0_2_LSB = 10;
parameter int MVIEN_HARD0_2_WIDTH = 3;
parameter logic [2:0] MVIEN_HARD0_2_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] MVIEN_HARD0_2_MASK = 64'h0000000000001C00;
parameter string MVIEN_HARD0_2_SW_TYPE = "WARL";

parameter int MVIEN_SEIP_MSB = 9;
parameter int MVIEN_SEIP_LSB = 9;
parameter int MVIEN_SEIP_WIDTH = 1;
parameter logic [0:0] MVIEN_SEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MVIEN_SEIP_MASK = 64'h0000000000000200;
parameter string MVIEN_SEIP_SW_TYPE = "WARL";

parameter int MVIEN_HARD0_1_MSB = 8;
parameter int MVIEN_HARD0_1_LSB = 2;
parameter int MVIEN_HARD0_1_WIDTH = 7;
parameter logic [6:0] MVIEN_HARD0_1_RESET = 64'h0000000000000000[8:2];
parameter logic [63:0] MVIEN_HARD0_1_MASK = 64'h00000000000001FC;
parameter string MVIEN_HARD0_1_SW_TYPE = "WARL";

parameter int MVIEN_SSIP_MSB = 1;
parameter int MVIEN_SSIP_LSB = 1;
parameter int MVIEN_SSIP_WIDTH = 1;
parameter logic [0:0] MVIEN_SSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MVIEN_SSIP_MASK = 64'h0000000000000002;
parameter string MVIEN_SSIP_SW_TYPE = "WARL";

parameter int MVIEN_HARD0_0_MSB = 0;
parameter int MVIEN_HARD0_0_LSB = 0;
parameter int MVIEN_HARD0_0_WIDTH = 1;
parameter logic [0:0] MVIEN_HARD0_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MVIEN_HARD0_0_MASK = 64'h0000000000000001;
parameter string MVIEN_HARD0_0_SW_TYPE = "WARL";


// MVIP CSR Field Defines
parameter int MVIP_LCOFIP_MSB = 13;
parameter int MVIP_LCOFIP_LSB = 13;
parameter int MVIP_LCOFIP_WIDTH = 1;
parameter logic [0:0] MVIP_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] MVIP_LCOFIP_MASK = 64'h0000000000002000;
parameter string MVIP_LCOFIP_SW_TYPE = "WARL";

parameter int MVIP_HARD0_3_MSB = 12;
parameter int MVIP_HARD0_3_LSB = 10;
parameter int MVIP_HARD0_3_WIDTH = 3;
parameter logic [2:0] MVIP_HARD0_3_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] MVIP_HARD0_3_MASK = 64'h0000000000001C00;
parameter string MVIP_HARD0_3_SW_TYPE = "WARL";

parameter int MVIP_SEIP_MSB = 9;
parameter int MVIP_SEIP_LSB = 9;
parameter int MVIP_SEIP_WIDTH = 1;
parameter logic [0:0] MVIP_SEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] MVIP_SEIP_MASK = 64'h0000000000000200;
parameter string MVIP_SEIP_SW_TYPE = "WARL";

parameter int MVIP_HARD0_2_MSB = 8;
parameter int MVIP_HARD0_2_LSB = 6;
parameter int MVIP_HARD0_2_WIDTH = 3;
parameter logic [2:0] MVIP_HARD0_2_RESET = 64'h0000000000000000[8:6];
parameter logic [63:0] MVIP_HARD0_2_MASK = 64'h00000000000001C0;
parameter string MVIP_HARD0_2_SW_TYPE = "WARL";

parameter int MVIP_STIP_MSB = 5;
parameter int MVIP_STIP_LSB = 5;
parameter int MVIP_STIP_WIDTH = 1;
parameter logic [0:0] MVIP_STIP_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] MVIP_STIP_MASK = 64'h0000000000000020;
parameter string MVIP_STIP_SW_TYPE = "WARL";

parameter int MVIP_HARD0_1_MSB = 4;
parameter int MVIP_HARD0_1_LSB = 2;
parameter int MVIP_HARD0_1_WIDTH = 3;
parameter logic [2:0] MVIP_HARD0_1_RESET = 64'h0000000000000000[4:2];
parameter logic [63:0] MVIP_HARD0_1_MASK = 64'h000000000000001C;
parameter string MVIP_HARD0_1_SW_TYPE = "WARL";

parameter int MVIP_SSIP_MSB = 1;
parameter int MVIP_SSIP_LSB = 1;
parameter int MVIP_SSIP_WIDTH = 1;
parameter logic [0:0] MVIP_SSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MVIP_SSIP_MASK = 64'h0000000000000002;
parameter string MVIP_SSIP_SW_TYPE = "WARL";

parameter int MVIP_HARD0_0_MSB = 0;
parameter int MVIP_HARD0_0_LSB = 0;
parameter int MVIP_HARD0_0_WIDTH = 1;
parameter logic [0:0] MVIP_HARD0_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MVIP_HARD0_0_MASK = 64'h0000000000000001;
parameter string MVIP_HARD0_0_SW_TYPE = "WARL";


// SSTATUS CSR Field Defines
parameter int SSTATUS_SD_MSB = 63;
parameter int SSTATUS_SD_LSB = 63;
parameter int SSTATUS_SD_WIDTH = 1;
parameter logic [0:0] SSTATUS_SD_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] SSTATUS_SD_MASK = 64'h8000000000000000;
parameter string SSTATUS_SD_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_6_MSB = 62;
parameter int SSTATUS_SSTATUS_WPRI_6_LSB = 34;
parameter int SSTATUS_SSTATUS_WPRI_6_WIDTH = 29;
parameter logic [28:0] SSTATUS_SSTATUS_WPRI_6_RESET = 64'h0000000000000000[62:34];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_6_MASK = 64'h7FFFFFFC00000000;
parameter string SSTATUS_SSTATUS_WPRI_6_SW_TYPE = "WPRI";

parameter int SSTATUS_UXL_MSB = 33;
parameter int SSTATUS_UXL_LSB = 32;
parameter int SSTATUS_UXL_WIDTH = 2;
parameter logic [1:0] SSTATUS_UXL_RESET = 64'h0000000000000002[33:32];
parameter logic [63:0] SSTATUS_UXL_MASK = 64'h0000000300000000;
parameter string SSTATUS_UXL_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_5_MSB = 31;
parameter int SSTATUS_SSTATUS_WPRI_5_LSB = 20;
parameter int SSTATUS_SSTATUS_WPRI_5_WIDTH = 12;
parameter logic [11:0] SSTATUS_SSTATUS_WPRI_5_RESET = 64'h0000000000000000[31:20];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_5_MASK = 64'h00000000FFF00000;
parameter string SSTATUS_SSTATUS_WPRI_5_SW_TYPE = "WPRI";

parameter int SSTATUS_MXR_MSB = 19;
parameter int SSTATUS_MXR_LSB = 19;
parameter int SSTATUS_MXR_WIDTH = 1;
parameter logic [0:0] SSTATUS_MXR_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] SSTATUS_MXR_MASK = 64'h0000000000080000;
parameter string SSTATUS_MXR_SW_TYPE = "WARL";

parameter int SSTATUS_SUM_MSB = 18;
parameter int SSTATUS_SUM_LSB = 18;
parameter int SSTATUS_SUM_WIDTH = 1;
parameter logic [0:0] SSTATUS_SUM_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] SSTATUS_SUM_MASK = 64'h0000000000040000;
parameter string SSTATUS_SUM_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_4_MSB = 17;
parameter int SSTATUS_SSTATUS_WPRI_4_LSB = 17;
parameter int SSTATUS_SSTATUS_WPRI_4_WIDTH = 1;
parameter logic [0:0] SSTATUS_SSTATUS_WPRI_4_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_4_MASK = 64'h0000000000020000;
parameter string SSTATUS_SSTATUS_WPRI_4_SW_TYPE = "WPRI";

parameter int SSTATUS_XS_MSB = 16;
parameter int SSTATUS_XS_LSB = 15;
parameter int SSTATUS_XS_WIDTH = 2;
parameter logic [1:0] SSTATUS_XS_RESET = 64'h0000000000000000[16:15];
parameter logic [63:0] SSTATUS_XS_MASK = 64'h0000000000018000;
parameter string SSTATUS_XS_SW_TYPE = "WARL";

parameter int SSTATUS_FS_MSB = 14;
parameter int SSTATUS_FS_LSB = 13;
parameter int SSTATUS_FS_WIDTH = 2;
parameter logic [1:0] SSTATUS_FS_RESET = 64'h0000000000000000[14:13];
parameter logic [63:0] SSTATUS_FS_MASK = 64'h0000000000006000;
parameter string SSTATUS_FS_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_3_MSB = 12;
parameter int SSTATUS_SSTATUS_WPRI_3_LSB = 11;
parameter int SSTATUS_SSTATUS_WPRI_3_WIDTH = 2;
parameter logic [1:0] SSTATUS_SSTATUS_WPRI_3_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_3_MASK = 64'h0000000000001800;
parameter string SSTATUS_SSTATUS_WPRI_3_SW_TYPE = "WPRI";

parameter int SSTATUS_VS_MSB = 10;
parameter int SSTATUS_VS_LSB = 9;
parameter int SSTATUS_VS_WIDTH = 2;
parameter logic [1:0] SSTATUS_VS_RESET = 64'h0000000000000000[10:9];
parameter logic [63:0] SSTATUS_VS_MASK = 64'h0000000000000600;
parameter string SSTATUS_VS_SW_TYPE = "WARL";

parameter int SSTATUS_SPP_MSB = 8;
parameter int SSTATUS_SPP_LSB = 8;
parameter int SSTATUS_SPP_WIDTH = 1;
parameter logic [0:0] SSTATUS_SPP_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] SSTATUS_SPP_MASK = 64'h0000000000000100;
parameter string SSTATUS_SPP_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_2_MSB = 7;
parameter int SSTATUS_SSTATUS_WPRI_2_LSB = 7;
parameter int SSTATUS_SSTATUS_WPRI_2_WIDTH = 1;
parameter logic [0:0] SSTATUS_SSTATUS_WPRI_2_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_2_MASK = 64'h0000000000000080;
parameter string SSTATUS_SSTATUS_WPRI_2_SW_TYPE = "WPRI";

parameter int SSTATUS_UBE_MSB = 6;
parameter int SSTATUS_UBE_LSB = 6;
parameter int SSTATUS_UBE_WIDTH = 1;
parameter logic [0:0] SSTATUS_UBE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] SSTATUS_UBE_MASK = 64'h0000000000000040;
parameter string SSTATUS_UBE_SW_TYPE = "WARL";

parameter int SSTATUS_SPIE_MSB = 5;
parameter int SSTATUS_SPIE_LSB = 5;
parameter int SSTATUS_SPIE_WIDTH = 1;
parameter logic [0:0] SSTATUS_SPIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] SSTATUS_SPIE_MASK = 64'h0000000000000020;
parameter string SSTATUS_SPIE_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_1_MSB = 4;
parameter int SSTATUS_SSTATUS_WPRI_1_LSB = 2;
parameter int SSTATUS_SSTATUS_WPRI_1_WIDTH = 3;
parameter logic [2:0] SSTATUS_SSTATUS_WPRI_1_RESET = 64'h0000000000000000[4:2];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_1_MASK = 64'h000000000000001C;
parameter string SSTATUS_SSTATUS_WPRI_1_SW_TYPE = "WPRI";

parameter int SSTATUS_SIE_MSB = 1;
parameter int SSTATUS_SIE_LSB = 1;
parameter int SSTATUS_SIE_WIDTH = 1;
parameter logic [0:0] SSTATUS_SIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] SSTATUS_SIE_MASK = 64'h0000000000000002;
parameter string SSTATUS_SIE_SW_TYPE = "WARL";

parameter int SSTATUS_SSTATUS_WPRI_0_MSB = 0;
parameter int SSTATUS_SSTATUS_WPRI_0_LSB = 0;
parameter int SSTATUS_SSTATUS_WPRI_0_WIDTH = 1;
parameter logic [0:0] SSTATUS_SSTATUS_WPRI_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] SSTATUS_SSTATUS_WPRI_0_MASK = 64'h0000000000000001;
parameter string SSTATUS_SSTATUS_WPRI_0_SW_TYPE = "WPRI";


// STVEC CSR Field Defines
parameter int STVEC_BASESXLEN12WARL_MSB = 63;
parameter int STVEC_BASESXLEN12WARL_LSB = 2;
parameter int STVEC_BASESXLEN12WARL_WIDTH = 62;
parameter logic [61:0] STVEC_BASESXLEN12WARL_RESET = 64'h0000000000000000[63:2];
parameter logic [63:0] STVEC_BASESXLEN12WARL_MASK = 64'hFFFFFFFFFFFFFFFC;
parameter string STVEC_BASESXLEN12WARL_SW_TYPE = "WARL";

parameter int STVEC_MODE_1_MSB = 1;
parameter int STVEC_MODE_1_LSB = 1;
parameter int STVEC_MODE_1_WIDTH = 1;
parameter logic [0:0] STVEC_MODE_1_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] STVEC_MODE_1_MASK = 64'h0000000000000002;
parameter string STVEC_MODE_1_SW_TYPE = "WARL";

parameter int STVEC_MODE_0_MSB = 0;
parameter int STVEC_MODE_0_LSB = 0;
parameter int STVEC_MODE_0_WIDTH = 1;
parameter logic [0:0] STVEC_MODE_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] STVEC_MODE_0_MASK = 64'h0000000000000001;
parameter string STVEC_MODE_0_SW_TYPE = "WARL";


// SIP CSR Field Defines
parameter int SIP_LCOFIP_MSB = 13;
parameter int SIP_LCOFIP_LSB = 13;
parameter int SIP_LCOFIP_WIDTH = 1;
parameter logic [0:0] SIP_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] SIP_LCOFIP_MASK = 64'h0000000000002000;
parameter string SIP_LCOFIP_SW_TYPE = "WARL";

parameter int SIP_HARD0_3_MSB = 12;
parameter int SIP_HARD0_3_LSB = 10;
parameter int SIP_HARD0_3_WIDTH = 3;
parameter logic [2:0] SIP_HARD0_3_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] SIP_HARD0_3_MASK = 64'h0000000000001C00;
parameter string SIP_HARD0_3_SW_TYPE = "WARL";

parameter int SIP_SEIP_MSB = 9;
parameter int SIP_SEIP_LSB = 9;
parameter int SIP_SEIP_WIDTH = 1;
parameter logic [0:0] SIP_SEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] SIP_SEIP_MASK = 64'h0000000000000200;
parameter string SIP_SEIP_SW_TYPE = "WARL";

parameter int SIP_HARD0_2_MSB = 7;
parameter int SIP_HARD0_2_LSB = 6;
parameter int SIP_HARD0_2_WIDTH = 2;
parameter logic [1:0] SIP_HARD0_2_RESET = 64'h0000000000000000[7:6];
parameter logic [63:0] SIP_HARD0_2_MASK = 64'h00000000000000C0;
parameter string SIP_HARD0_2_SW_TYPE = "WARL";

parameter int SIP_STIP_MSB = 5;
parameter int SIP_STIP_LSB = 5;
parameter int SIP_STIP_WIDTH = 1;
parameter logic [0:0] SIP_STIP_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] SIP_STIP_MASK = 64'h0000000000000020;
parameter string SIP_STIP_SW_TYPE = "WARL";

parameter int SIP_HARD0_1_MSB = 3;
parameter int SIP_HARD0_1_LSB = 2;
parameter int SIP_HARD0_1_WIDTH = 2;
parameter logic [1:0] SIP_HARD0_1_RESET = 64'h0000000000000000[3:2];
parameter logic [63:0] SIP_HARD0_1_MASK = 64'h000000000000000C;
parameter string SIP_HARD0_1_SW_TYPE = "WARL";

parameter int SIP_SSIP_MSB = 1;
parameter int SIP_SSIP_LSB = 1;
parameter int SIP_SSIP_WIDTH = 1;
parameter logic [0:0] SIP_SSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] SIP_SSIP_MASK = 64'h0000000000000002;
parameter string SIP_SSIP_SW_TYPE = "WARL";


// SIE CSR Field Defines
parameter int SIE_LCOFIE_MSB = 13;
parameter int SIE_LCOFIE_LSB = 13;
parameter int SIE_LCOFIE_WIDTH = 1;
parameter logic [0:0] SIE_LCOFIE_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] SIE_LCOFIE_MASK = 64'h0000000000002000;
parameter string SIE_LCOFIE_SW_TYPE = "WARL";

parameter int SIE_HARD0_2_MSB = 12;
parameter int SIE_HARD0_2_LSB = 10;
parameter int SIE_HARD0_2_WIDTH = 3;
parameter logic [2:0] SIE_HARD0_2_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] SIE_HARD0_2_MASK = 64'h0000000000001C00;
parameter string SIE_HARD0_2_SW_TYPE = "WARL";

parameter int SIE_SEIE_MSB = 9;
parameter int SIE_SEIE_LSB = 9;
parameter int SIE_SEIE_WIDTH = 1;
parameter logic [0:0] SIE_SEIE_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] SIE_SEIE_MASK = 64'h0000000000000200;
parameter string SIE_SEIE_SW_TYPE = "WARL";

parameter int SIE_HARD0_1_MSB = 7;
parameter int SIE_HARD0_1_LSB = 6;
parameter int SIE_HARD0_1_WIDTH = 2;
parameter logic [1:0] SIE_HARD0_1_RESET = 64'h0000000000000000[7:6];
parameter logic [63:0] SIE_HARD0_1_MASK = 64'h00000000000000C0;
parameter string SIE_HARD0_1_SW_TYPE = "WARL";

parameter int SIE_STIE_MSB = 5;
parameter int SIE_STIE_LSB = 5;
parameter int SIE_STIE_WIDTH = 1;
parameter logic [0:0] SIE_STIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] SIE_STIE_MASK = 64'h0000000000000020;
parameter string SIE_STIE_SW_TYPE = "WARL";

parameter int SIE_HARD0_0_MSB = 3;
parameter int SIE_HARD0_0_LSB = 2;
parameter int SIE_HARD0_0_WIDTH = 2;
parameter logic [1:0] SIE_HARD0_0_RESET = 64'h0000000000000000[3:2];
parameter logic [63:0] SIE_HARD0_0_MASK = 64'h000000000000000C;
parameter string SIE_HARD0_0_SW_TYPE = "WARL";

parameter int SIE_SSIE_MSB = 1;
parameter int SIE_SSIE_LSB = 1;
parameter int SIE_SSIE_WIDTH = 1;
parameter logic [0:0] SIE_SSIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] SIE_SSIE_MASK = 64'h0000000000000002;
parameter string SIE_SSIE_SW_TYPE = "WARL";


// SCOUNTEREN CSR Field Defines
parameter int SCOUNTEREN_HPM31_MSB = 31;
parameter int SCOUNTEREN_HPM31_LSB = 31;
parameter int SCOUNTEREN_HPM31_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM31_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] SCOUNTEREN_HPM31_MASK = 64'h0000000080000000;
parameter string SCOUNTEREN_HPM31_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM30_MSB = 30;
parameter int SCOUNTEREN_HPM30_LSB = 30;
parameter int SCOUNTEREN_HPM30_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM30_RESET = 64'h0000000000000000[30:30];
parameter logic [63:0] SCOUNTEREN_HPM30_MASK = 64'h0000000040000000;
parameter string SCOUNTEREN_HPM30_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM29_MSB = 29;
parameter int SCOUNTEREN_HPM29_LSB = 29;
parameter int SCOUNTEREN_HPM29_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM29_RESET = 64'h0000000000000000[29:29];
parameter logic [63:0] SCOUNTEREN_HPM29_MASK = 64'h0000000020000000;
parameter string SCOUNTEREN_HPM29_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM28_MSB = 28;
parameter int SCOUNTEREN_HPM28_LSB = 28;
parameter int SCOUNTEREN_HPM28_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM28_RESET = 64'h0000000000000000[28:28];
parameter logic [63:0] SCOUNTEREN_HPM28_MASK = 64'h0000000010000000;
parameter string SCOUNTEREN_HPM28_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM27_MSB = 27;
parameter int SCOUNTEREN_HPM27_LSB = 27;
parameter int SCOUNTEREN_HPM27_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM27_RESET = 64'h0000000000000000[27:27];
parameter logic [63:0] SCOUNTEREN_HPM27_MASK = 64'h0000000008000000;
parameter string SCOUNTEREN_HPM27_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM26_MSB = 26;
parameter int SCOUNTEREN_HPM26_LSB = 26;
parameter int SCOUNTEREN_HPM26_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM26_RESET = 64'h0000000000000000[26:26];
parameter logic [63:0] SCOUNTEREN_HPM26_MASK = 64'h0000000004000000;
parameter string SCOUNTEREN_HPM26_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM25_MSB = 25;
parameter int SCOUNTEREN_HPM25_LSB = 25;
parameter int SCOUNTEREN_HPM25_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM25_RESET = 64'h0000000000000000[25:25];
parameter logic [63:0] SCOUNTEREN_HPM25_MASK = 64'h0000000002000000;
parameter string SCOUNTEREN_HPM25_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM24_MSB = 24;
parameter int SCOUNTEREN_HPM24_LSB = 24;
parameter int SCOUNTEREN_HPM24_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM24_RESET = 64'h0000000000000000[24:24];
parameter logic [63:0] SCOUNTEREN_HPM24_MASK = 64'h0000000001000000;
parameter string SCOUNTEREN_HPM24_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM23_MSB = 23;
parameter int SCOUNTEREN_HPM23_LSB = 23;
parameter int SCOUNTEREN_HPM23_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM23_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] SCOUNTEREN_HPM23_MASK = 64'h0000000000800000;
parameter string SCOUNTEREN_HPM23_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM22_MSB = 22;
parameter int SCOUNTEREN_HPM22_LSB = 22;
parameter int SCOUNTEREN_HPM22_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM22_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] SCOUNTEREN_HPM22_MASK = 64'h0000000000400000;
parameter string SCOUNTEREN_HPM22_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM21_MSB = 21;
parameter int SCOUNTEREN_HPM21_LSB = 21;
parameter int SCOUNTEREN_HPM21_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM21_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] SCOUNTEREN_HPM21_MASK = 64'h0000000000200000;
parameter string SCOUNTEREN_HPM21_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM20_MSB = 20;
parameter int SCOUNTEREN_HPM20_LSB = 20;
parameter int SCOUNTEREN_HPM20_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM20_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] SCOUNTEREN_HPM20_MASK = 64'h0000000000100000;
parameter string SCOUNTEREN_HPM20_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM19_MSB = 19;
parameter int SCOUNTEREN_HPM19_LSB = 19;
parameter int SCOUNTEREN_HPM19_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM19_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] SCOUNTEREN_HPM19_MASK = 64'h0000000000080000;
parameter string SCOUNTEREN_HPM19_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM18_MSB = 18;
parameter int SCOUNTEREN_HPM18_LSB = 18;
parameter int SCOUNTEREN_HPM18_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM18_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] SCOUNTEREN_HPM18_MASK = 64'h0000000000040000;
parameter string SCOUNTEREN_HPM18_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM17_MSB = 17;
parameter int SCOUNTEREN_HPM17_LSB = 17;
parameter int SCOUNTEREN_HPM17_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM17_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] SCOUNTEREN_HPM17_MASK = 64'h0000000000020000;
parameter string SCOUNTEREN_HPM17_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM16_MSB = 16;
parameter int SCOUNTEREN_HPM16_LSB = 16;
parameter int SCOUNTEREN_HPM16_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM16_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] SCOUNTEREN_HPM16_MASK = 64'h0000000000010000;
parameter string SCOUNTEREN_HPM16_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM15_MSB = 15;
parameter int SCOUNTEREN_HPM15_LSB = 15;
parameter int SCOUNTEREN_HPM15_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM15_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] SCOUNTEREN_HPM15_MASK = 64'h0000000000008000;
parameter string SCOUNTEREN_HPM15_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM14_MSB = 14;
parameter int SCOUNTEREN_HPM14_LSB = 14;
parameter int SCOUNTEREN_HPM14_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM14_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] SCOUNTEREN_HPM14_MASK = 64'h0000000000004000;
parameter string SCOUNTEREN_HPM14_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM13_MSB = 13;
parameter int SCOUNTEREN_HPM13_LSB = 13;
parameter int SCOUNTEREN_HPM13_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM13_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] SCOUNTEREN_HPM13_MASK = 64'h0000000000002000;
parameter string SCOUNTEREN_HPM13_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM12_MSB = 12;
parameter int SCOUNTEREN_HPM12_LSB = 12;
parameter int SCOUNTEREN_HPM12_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM12_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] SCOUNTEREN_HPM12_MASK = 64'h0000000000001000;
parameter string SCOUNTEREN_HPM12_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM11_MSB = 11;
parameter int SCOUNTEREN_HPM11_LSB = 11;
parameter int SCOUNTEREN_HPM11_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM11_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] SCOUNTEREN_HPM11_MASK = 64'h0000000000000800;
parameter string SCOUNTEREN_HPM11_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM10_MSB = 10;
parameter int SCOUNTEREN_HPM10_LSB = 10;
parameter int SCOUNTEREN_HPM10_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM10_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] SCOUNTEREN_HPM10_MASK = 64'h0000000000000400;
parameter string SCOUNTEREN_HPM10_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM9_MSB = 9;
parameter int SCOUNTEREN_HPM9_LSB = 9;
parameter int SCOUNTEREN_HPM9_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM9_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] SCOUNTEREN_HPM9_MASK = 64'h0000000000000200;
parameter string SCOUNTEREN_HPM9_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM8_MSB = 8;
parameter int SCOUNTEREN_HPM8_LSB = 8;
parameter int SCOUNTEREN_HPM8_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM8_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] SCOUNTEREN_HPM8_MASK = 64'h0000000000000100;
parameter string SCOUNTEREN_HPM8_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM7_MSB = 7;
parameter int SCOUNTEREN_HPM7_LSB = 7;
parameter int SCOUNTEREN_HPM7_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM7_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] SCOUNTEREN_HPM7_MASK = 64'h0000000000000080;
parameter string SCOUNTEREN_HPM7_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM6_MSB = 6;
parameter int SCOUNTEREN_HPM6_LSB = 6;
parameter int SCOUNTEREN_HPM6_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM6_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] SCOUNTEREN_HPM6_MASK = 64'h0000000000000040;
parameter string SCOUNTEREN_HPM6_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM5_MSB = 5;
parameter int SCOUNTEREN_HPM5_LSB = 5;
parameter int SCOUNTEREN_HPM5_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM5_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] SCOUNTEREN_HPM5_MASK = 64'h0000000000000020;
parameter string SCOUNTEREN_HPM5_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM4_MSB = 4;
parameter int SCOUNTEREN_HPM4_LSB = 4;
parameter int SCOUNTEREN_HPM4_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM4_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] SCOUNTEREN_HPM4_MASK = 64'h0000000000000010;
parameter string SCOUNTEREN_HPM4_SW_TYPE = "WARL";

parameter int SCOUNTEREN_HPM3_MSB = 3;
parameter int SCOUNTEREN_HPM3_LSB = 3;
parameter int SCOUNTEREN_HPM3_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_HPM3_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] SCOUNTEREN_HPM3_MASK = 64'h0000000000000008;
parameter string SCOUNTEREN_HPM3_SW_TYPE = "WARL";

parameter int SCOUNTEREN_IR_MSB = 2;
parameter int SCOUNTEREN_IR_LSB = 2;
parameter int SCOUNTEREN_IR_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_IR_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] SCOUNTEREN_IR_MASK = 64'h0000000000000004;
parameter string SCOUNTEREN_IR_SW_TYPE = "WARL";

parameter int SCOUNTEREN_TM_MSB = 1;
parameter int SCOUNTEREN_TM_LSB = 1;
parameter int SCOUNTEREN_TM_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_TM_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] SCOUNTEREN_TM_MASK = 64'h0000000000000002;
parameter string SCOUNTEREN_TM_SW_TYPE = "WARL";

parameter int SCOUNTEREN_CY_MSB = 0;
parameter int SCOUNTEREN_CY_LSB = 0;
parameter int SCOUNTEREN_CY_WIDTH = 1;
parameter logic [0:0] SCOUNTEREN_CY_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] SCOUNTEREN_CY_MASK = 64'h0000000000000001;
parameter string SCOUNTEREN_CY_SW_TYPE = "WARL";


// SSCRATCH CSR Field Defines
parameter int SSCRATCH_SSCRATCH_MSB = 63;
parameter int SSCRATCH_SSCRATCH_LSB = 0;
parameter int SSCRATCH_SSCRATCH_WIDTH = 64;
parameter logic [63:0] SSCRATCH_SSCRATCH_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] SSCRATCH_SSCRATCH_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string SSCRATCH_SSCRATCH_SW_TYPE = "WARL";


// SEPC CSR Field Defines
parameter int SEPC_ADDR_MSB = 63;
parameter int SEPC_ADDR_LSB = 1;
parameter int SEPC_ADDR_WIDTH = 63;
parameter logic [62:0] SEPC_ADDR_RESET = 64'h0000000000000000[63:1];
parameter logic [63:0] SEPC_ADDR_MASK = 64'hFFFFFFFFFFFFFFFE;
parameter string SEPC_ADDR_SW_TYPE = "WARL";


// SCAUSE CSR Field Defines
parameter int SCAUSE_INTERRUPT_MSB = 63;
parameter int SCAUSE_INTERRUPT_LSB = 63;
parameter int SCAUSE_INTERRUPT_WIDTH = 1;
parameter logic [0:0] SCAUSE_INTERRUPT_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] SCAUSE_INTERRUPT_MASK = 64'h8000000000000000;
parameter string SCAUSE_INTERRUPT_SW_TYPE = "WARL";

parameter int SCAUSE_EXCEPTIONCODEWLRL_MSB = 62;
parameter int SCAUSE_EXCEPTIONCODEWLRL_LSB = 0;
parameter int SCAUSE_EXCEPTIONCODEWLRL_WIDTH = 63;
parameter logic [62:0] SCAUSE_EXCEPTIONCODEWLRL_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] SCAUSE_EXCEPTIONCODEWLRL_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string SCAUSE_EXCEPTIONCODEWLRL_SW_TYPE = "WARL";


// STVAL CSR Field Defines
parameter int STVAL_STVAL_MSB = 63;
parameter int STVAL_STVAL_LSB = 0;
parameter int STVAL_STVAL_WIDTH = 64;
parameter logic [63:0] STVAL_STVAL_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] STVAL_STVAL_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string STVAL_STVAL_SW_TYPE = "WARL";


// STIMECMP CSR Field Defines
parameter int STIMECMP_STIMECMP_MSB = 63;
parameter int STIMECMP_STIMECMP_LSB = 0;
parameter int STIMECMP_STIMECMP_WIDTH = 64;
parameter logic [63:0] STIMECMP_STIMECMP_RESET = 64'h00000000FFFFFFFF[63:0];
parameter logic [63:0] STIMECMP_STIMECMP_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string STIMECMP_STIMECMP_SW_TYPE = "WARL";


// SENVCFG CSR Field Defines
parameter int SENVCFG_WPRI_2_MSB = 63;
parameter int SENVCFG_WPRI_2_LSB = 34;
parameter int SENVCFG_WPRI_2_WIDTH = 30;
parameter logic [29:0] SENVCFG_WPRI_2_RESET = 64'h0000000000000000[63:34];
parameter logic [63:0] SENVCFG_WPRI_2_MASK = 64'hFFFFFFFC00000000;
parameter string SENVCFG_WPRI_2_SW_TYPE = "WPRI";

parameter int SENVCFG_PMM_MSB = 33;
parameter int SENVCFG_PMM_LSB = 32;
parameter int SENVCFG_PMM_WIDTH = 2;
parameter logic [1:0] SENVCFG_PMM_RESET = 64'h0000000000000000[33:32];
parameter logic [63:0] SENVCFG_PMM_MASK = 64'h0000000300000000;
parameter string SENVCFG_PMM_SW_TYPE = "WARL";

parameter int SENVCFG_WPRI_1_MSB = 31;
parameter int SENVCFG_WPRI_1_LSB = 8;
parameter int SENVCFG_WPRI_1_WIDTH = 24;
parameter logic [23:0] SENVCFG_WPRI_1_RESET = 64'h0000000000000000[31:8];
parameter logic [63:0] SENVCFG_WPRI_1_MASK = 64'h00000000FFFFFF00;
parameter string SENVCFG_WPRI_1_SW_TYPE = "WPRI";

parameter int SENVCFG_CBZE_MSB = 7;
parameter int SENVCFG_CBZE_LSB = 7;
parameter int SENVCFG_CBZE_WIDTH = 1;
parameter logic [0:0] SENVCFG_CBZE_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] SENVCFG_CBZE_MASK = 64'h0000000000000080;
parameter string SENVCFG_CBZE_SW_TYPE = "WARL";

parameter int SENVCFG_CBCFE_MSB = 6;
parameter int SENVCFG_CBCFE_LSB = 6;
parameter int SENVCFG_CBCFE_WIDTH = 1;
parameter logic [0:0] SENVCFG_CBCFE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] SENVCFG_CBCFE_MASK = 64'h0000000000000040;
parameter string SENVCFG_CBCFE_SW_TYPE = "WARL";

parameter int SENVCFG_CBIE_MSB = 5;
parameter int SENVCFG_CBIE_LSB = 4;
parameter int SENVCFG_CBIE_WIDTH = 2;
parameter logic [1:0] SENVCFG_CBIE_RESET = 64'h0000000000000000[5:4];
parameter logic [63:0] SENVCFG_CBIE_MASK = 64'h0000000000000030;
parameter string SENVCFG_CBIE_SW_TYPE = "WARL";

parameter int SENVCFG_WPRI_0_MSB = 3;
parameter int SENVCFG_WPRI_0_LSB = 1;
parameter int SENVCFG_WPRI_0_WIDTH = 3;
parameter logic [2:0] SENVCFG_WPRI_0_RESET = 64'h0000000000000000[3:1];
parameter logic [63:0] SENVCFG_WPRI_0_MASK = 64'h000000000000000E;
parameter string SENVCFG_WPRI_0_SW_TYPE = "WPRI";

parameter int SENVCFG_FIOM_MSB = 0;
parameter int SENVCFG_FIOM_LSB = 0;
parameter int SENVCFG_FIOM_WIDTH = 1;
parameter logic [0:0] SENVCFG_FIOM_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] SENVCFG_FIOM_MASK = 64'h0000000000000001;
parameter string SENVCFG_FIOM_SW_TYPE = "WARL";


// SATP CSR Field Defines
parameter int SATP_MODE_MSB = 63;
parameter int SATP_MODE_LSB = 60;
parameter int SATP_MODE_WIDTH = 4;
parameter logic [3:0] SATP_MODE_RESET = 64'h0000000000000000[63:60];
parameter logic [63:0] SATP_MODE_MASK = 64'hF000000000000000;
parameter string SATP_MODE_SW_TYPE = "WARL";

parameter int SATP_ASID_MSB = 59;
parameter int SATP_ASID_LSB = 44;
parameter int SATP_ASID_WIDTH = 16;
parameter logic [15:0] SATP_ASID_RESET = 64'h0000000000000000[59:44];
parameter logic [63:0] SATP_ASID_MASK = 64'h0FFFF00000000000;
parameter string SATP_ASID_SW_TYPE = "WARL";

parameter int SATP_PPN_MSB = 43;
parameter int SATP_PPN_LSB = 0;
parameter int SATP_PPN_WIDTH = 44;
parameter logic [43:0] SATP_PPN_RESET = 64'h0000000000000000[43:0];
parameter logic [63:0] SATP_PPN_MASK = 64'h00000FFFFFFFFFFF;
parameter string SATP_PPN_SW_TYPE = "WARL";


// SRMCFG CSR Field Defines
parameter int SRMCFG_MCID_MSB = 27;
parameter int SRMCFG_MCID_LSB = 16;
parameter int SRMCFG_MCID_WIDTH = 12;
parameter logic [11:0] SRMCFG_MCID_RESET = 64'h0000000000000000[27:16];
parameter logic [63:0] SRMCFG_MCID_MASK = 64'h000000000FFF0000;
parameter string SRMCFG_MCID_SW_TYPE = "WARL";

parameter int SRMCFG_RCID_MSB = 11;
parameter int SRMCFG_RCID_LSB = 0;
parameter int SRMCFG_RCID_WIDTH = 12;
parameter logic [11:0] SRMCFG_RCID_RESET = 64'h0000000000000000[11:0];
parameter logic [63:0] SRMCFG_RCID_MASK = 64'h0000000000000FFF;
parameter string SRMCFG_RCID_SW_TYPE = "WARL";


// SISELECT CSR Field Defines
parameter int SISELECT_RSVD_63_9_MSB = 63;
parameter int SISELECT_RSVD_63_9_LSB = 9;
parameter int SISELECT_RSVD_63_9_WIDTH = 55;
parameter logic [54:0] SISELECT_RSVD_63_9_RESET = 64'h0000000000000000[63:9];
parameter logic [63:0] SISELECT_RSVD_63_9_MASK = 64'hFFFFFFFFFFFFFE00;
parameter string SISELECT_RSVD_63_9_SW_TYPE = "WARL";

parameter int SISELECT_INTERRUPTS_MSB = 8;
parameter int SISELECT_INTERRUPTS_LSB = 0;
parameter int SISELECT_INTERRUPTS_WIDTH = 9;
parameter logic [8:0] SISELECT_INTERRUPTS_RESET = 64'h0000000000000000[8:0];
parameter logic [63:0] SISELECT_INTERRUPTS_MASK = 64'h00000000000001FF;
parameter string SISELECT_INTERRUPTS_SW_TYPE = "WARL";


// SIREG CSR Field Defines
parameter int SIREG_SIREG_MSB = 63;
parameter int SIREG_SIREG_LSB = 0;
parameter int SIREG_SIREG_WIDTH = 64;
parameter logic [63:0] SIREG_SIREG_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] SIREG_SIREG_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string SIREG_SIREG_SW_TYPE = "WARL";


// STOPEI CSR Field Defines
parameter int STOPEI_RSVD_63_27_MSB = 63;
parameter int STOPEI_RSVD_63_27_LSB = 27;
parameter int STOPEI_RSVD_63_27_WIDTH = 37;
parameter logic [36:0] STOPEI_RSVD_63_27_RESET = 64'h0000000000000000[63:27];
parameter logic [63:0] STOPEI_RSVD_63_27_MASK = 64'hFFFFFFFFF8000000;
parameter string STOPEI_RSVD_63_27_SW_TYPE = "WARL";

parameter int STOPEI_IDENTITY_MSB = 26;
parameter int STOPEI_IDENTITY_LSB = 16;
parameter int STOPEI_IDENTITY_WIDTH = 11;
parameter logic [10:0] STOPEI_IDENTITY_RESET = 64'h0000000000000000[26:16];
parameter logic [63:0] STOPEI_IDENTITY_MASK = 64'h0000000007FF0000;
parameter string STOPEI_IDENTITY_SW_TYPE = "WARL";

parameter int STOPEI_RSVD_15_11_MSB = 15;
parameter int STOPEI_RSVD_15_11_LSB = 11;
parameter int STOPEI_RSVD_15_11_WIDTH = 5;
parameter logic [4:0] STOPEI_RSVD_15_11_RESET = 64'h0000000000000000[15:11];
parameter logic [63:0] STOPEI_RSVD_15_11_MASK = 64'h000000000000F800;
parameter string STOPEI_RSVD_15_11_SW_TYPE = "WARL";

parameter int STOPEI_PRIORITY_MSB = 10;
parameter int STOPEI_PRIORITY_LSB = 0;
parameter int STOPEI_PRIORITY_WIDTH = 11;
parameter logic [10:0] STOPEI_PRIORITY_RESET = 64'h0000000000000000[10:0];
parameter logic [63:0] STOPEI_PRIORITY_MASK = 64'h00000000000007FF;
parameter string STOPEI_PRIORITY_SW_TYPE = "WARL";


// STOPI CSR Field Defines
parameter int STOPI_RSVD_63_28_MSB = 63;
parameter int STOPI_RSVD_63_28_LSB = 28;
parameter int STOPI_RSVD_63_28_WIDTH = 36;
parameter logic [35:0] STOPI_RSVD_63_28_RESET = 64'h0000000000000000[63:28];
parameter logic [63:0] STOPI_RSVD_63_28_MASK = 64'hFFFFFFFFF0000000;
parameter string STOPI_RSVD_63_28_SW_TYPE = "WARL";

parameter int STOPI_IID_MSB = 27;
parameter int STOPI_IID_LSB = 16;
parameter int STOPI_IID_WIDTH = 12;
parameter logic [11:0] STOPI_IID_RESET = 64'h0000000000000000[27:16];
parameter logic [63:0] STOPI_IID_MASK = 64'h000000000FFF0000;
parameter string STOPI_IID_SW_TYPE = "WARL";

parameter int STOPI_RSVD_15_8_MSB = 15;
parameter int STOPI_RSVD_15_8_LSB = 8;
parameter int STOPI_RSVD_15_8_WIDTH = 8;
parameter logic [7:0] STOPI_RSVD_15_8_RESET = 64'h0000000000000000[15:8];
parameter logic [63:0] STOPI_RSVD_15_8_MASK = 64'h000000000000FF00;
parameter string STOPI_RSVD_15_8_SW_TYPE = "WARL";

parameter int STOPI_IPRIO_MSB = 7;
parameter int STOPI_IPRIO_LSB = 0;
parameter int STOPI_IPRIO_WIDTH = 8;
parameter logic [7:0] STOPI_IPRIO_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] STOPI_IPRIO_MASK = 64'h00000000000000FF;
parameter string STOPI_IPRIO_SW_TYPE = "WARL";


// SEED CSR Field Defines
parameter int SEED_OPST_MSB = 31;
parameter int SEED_OPST_LSB = 30;
parameter int SEED_OPST_WIDTH = 2;
parameter logic [1:0] SEED_OPST_RESET = 64'h0000000000000001[31:30];
parameter logic [63:0] SEED_OPST_MASK = 64'h00000000C0000000;
parameter string SEED_OPST_SW_TYPE = "WARL";

parameter int SEED_RSVD_29_24_MSB = 29;
parameter int SEED_RSVD_29_24_LSB = 24;
parameter int SEED_RSVD_29_24_WIDTH = 6;
parameter logic [5:0] SEED_RSVD_29_24_RESET = 64'h0000000000000000[29:24];
parameter logic [63:0] SEED_RSVD_29_24_MASK = 64'h000000003F000000;
parameter string SEED_RSVD_29_24_SW_TYPE = "WARL";

parameter int SEED_CUSTOM_MSB = 23;
parameter int SEED_CUSTOM_LSB = 16;
parameter int SEED_CUSTOM_WIDTH = 8;
parameter logic [7:0] SEED_CUSTOM_RESET = 64'h0000000000000000[23:16];
parameter logic [63:0] SEED_CUSTOM_MASK = 64'h0000000000FF0000;
parameter string SEED_CUSTOM_SW_TYPE = "WARL";

parameter int SEED_ENTROPY_MSB = 15;
parameter int SEED_ENTROPY_LSB = 0;
parameter int SEED_ENTROPY_WIDTH = 16;
parameter logic [15:0] SEED_ENTROPY_RESET = 64'h0000000000000000[15:0];
parameter logic [63:0] SEED_ENTROPY_MASK = 64'h000000000000FFFF;
parameter string SEED_ENTROPY_SW_TYPE = "WARL";


// FFLAGS CSR Field Defines
parameter int FFLAGS_NV_MSB = 4;
parameter int FFLAGS_NV_LSB = 4;
parameter int FFLAGS_NV_WIDTH = 1;
parameter logic [0:0] FFLAGS_NV_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] FFLAGS_NV_MASK = 64'h0000000000000010;
parameter string FFLAGS_NV_SW_TYPE = "WARL";

parameter int FFLAGS_DZ_MSB = 3;
parameter int FFLAGS_DZ_LSB = 3;
parameter int FFLAGS_DZ_WIDTH = 1;
parameter logic [0:0] FFLAGS_DZ_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] FFLAGS_DZ_MASK = 64'h0000000000000008;
parameter string FFLAGS_DZ_SW_TYPE = "WARL";

parameter int FFLAGS_OF_MSB = 2;
parameter int FFLAGS_OF_LSB = 2;
parameter int FFLAGS_OF_WIDTH = 1;
parameter logic [0:0] FFLAGS_OF_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] FFLAGS_OF_MASK = 64'h0000000000000004;
parameter string FFLAGS_OF_SW_TYPE = "WARL";

parameter int FFLAGS_UF_MSB = 1;
parameter int FFLAGS_UF_LSB = 1;
parameter int FFLAGS_UF_WIDTH = 1;
parameter logic [0:0] FFLAGS_UF_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] FFLAGS_UF_MASK = 64'h0000000000000002;
parameter string FFLAGS_UF_SW_TYPE = "WARL";

parameter int FFLAGS_NX_MSB = 0;
parameter int FFLAGS_NX_LSB = 0;
parameter int FFLAGS_NX_WIDTH = 1;
parameter logic [0:0] FFLAGS_NX_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] FFLAGS_NX_MASK = 64'h0000000000000001;
parameter string FFLAGS_NX_SW_TYPE = "WARL";


// FRM CSR Field Defines
parameter int FRM_FRM_MSB = 2;
parameter int FRM_FRM_LSB = 0;
parameter int FRM_FRM_WIDTH = 3;
parameter logic [2:0] FRM_FRM_RESET = 64'h0000000000000000[2:0];
parameter logic [63:0] FRM_FRM_MASK = 64'h0000000000000007;
parameter string FRM_FRM_SW_TYPE = "WARL";


// FCSR CSR Field Defines
parameter int FCSR_RESERVED_MSB = 31;
parameter int FCSR_RESERVED_LSB = 8;
parameter int FCSR_RESERVED_WIDTH = 24;
parameter logic [23:0] FCSR_RESERVED_RESET = 64'h0000000000000000[31:8];
parameter logic [63:0] FCSR_RESERVED_MASK = 64'h00000000FFFFFF00;
parameter string FCSR_RESERVED_SW_TYPE = "WARL";

parameter int FCSR_FRM_MSB = 7;
parameter int FCSR_FRM_LSB = 5;
parameter int FCSR_FRM_WIDTH = 3;
parameter logic [2:0] FCSR_FRM_RESET = 64'h0000000000000000[7:5];
parameter logic [63:0] FCSR_FRM_MASK = 64'h00000000000000E0;
parameter string FCSR_FRM_SW_TYPE = "WARL";

parameter int FCSR_NV_MSB = 4;
parameter int FCSR_NV_LSB = 4;
parameter int FCSR_NV_WIDTH = 1;
parameter logic [0:0] FCSR_NV_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] FCSR_NV_MASK = 64'h0000000000000010;
parameter string FCSR_NV_SW_TYPE = "WARL";

parameter int FCSR_DZ_MSB = 3;
parameter int FCSR_DZ_LSB = 3;
parameter int FCSR_DZ_WIDTH = 1;
parameter logic [0:0] FCSR_DZ_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] FCSR_DZ_MASK = 64'h0000000000000008;
parameter string FCSR_DZ_SW_TYPE = "WARL";

parameter int FCSR_OF_MSB = 2;
parameter int FCSR_OF_LSB = 2;
parameter int FCSR_OF_WIDTH = 1;
parameter logic [0:0] FCSR_OF_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] FCSR_OF_MASK = 64'h0000000000000004;
parameter string FCSR_OF_SW_TYPE = "WARL";

parameter int FCSR_UF_MSB = 1;
parameter int FCSR_UF_LSB = 1;
parameter int FCSR_UF_WIDTH = 1;
parameter logic [0:0] FCSR_UF_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] FCSR_UF_MASK = 64'h0000000000000002;
parameter string FCSR_UF_SW_TYPE = "WARL";

parameter int FCSR_NX_MSB = 0;
parameter int FCSR_NX_LSB = 0;
parameter int FCSR_NX_WIDTH = 1;
parameter logic [0:0] FCSR_NX_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] FCSR_NX_MASK = 64'h0000000000000001;
parameter string FCSR_NX_SW_TYPE = "WARL";


// VSTART CSR Field Defines
parameter int VSTART_VSTART_MSB = 7;
parameter int VSTART_VSTART_LSB = 0;
parameter int VSTART_VSTART_WIDTH = 8;
parameter logic [7:0] VSTART_VSTART_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] VSTART_VSTART_MASK = 64'h00000000000000FF;
parameter string VSTART_VSTART_SW_TYPE = "WARL";


// VXSAT CSR Field Defines
parameter int VXSAT_VXSAT_MSB = 0;
parameter int VXSAT_VXSAT_LSB = 0;
parameter int VXSAT_VXSAT_WIDTH = 1;
parameter logic [0:0] VXSAT_VXSAT_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] VXSAT_VXSAT_MASK = 64'h0000000000000001;
parameter string VXSAT_VXSAT_SW_TYPE = "WARL";


// VXRM CSR Field Defines
parameter int VXRM_VXRM_MSB = 1;
parameter int VXRM_VXRM_LSB = 0;
parameter int VXRM_VXRM_WIDTH = 2;
parameter logic [1:0] VXRM_VXRM_RESET = 64'h0000000000000000[1:0];
parameter logic [63:0] VXRM_VXRM_MASK = 64'h0000000000000003;
parameter string VXRM_VXRM_SW_TYPE = "WARL";


// VCSR CSR Field Defines
parameter int VCSR_VXRM_MSB = 2;
parameter int VCSR_VXRM_LSB = 1;
parameter int VCSR_VXRM_WIDTH = 2;
parameter logic [1:0] VCSR_VXRM_RESET = 64'h0000000000000000[2:1];
parameter logic [63:0] VCSR_VXRM_MASK = 64'h0000000000000006;
parameter string VCSR_VXRM_SW_TYPE = "WARL";

parameter int VCSR_VXSAT_MSB = 0;
parameter int VCSR_VXSAT_LSB = 0;
parameter int VCSR_VXSAT_WIDTH = 1;
parameter logic [0:0] VCSR_VXSAT_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] VCSR_VXSAT_MASK = 64'h0000000000000001;
parameter string VCSR_VXSAT_SW_TYPE = "WARL";


// VL CSR Field Defines
parameter int VL_VL_MSB = 63;
parameter int VL_VL_LSB = 0;
parameter int VL_VL_WIDTH = 64;
parameter logic [63:0] VL_VL_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] VL_VL_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VL_VL_SW_TYPE = "WARL";


// VTYPE CSR Field Defines
parameter int VTYPE_VILL_MSB = 63;
parameter int VTYPE_VILL_LSB = 63;
parameter int VTYPE_VILL_WIDTH = 1;
parameter logic [0:0] VTYPE_VILL_RESET = 64'h0000000000000001[63:63];
parameter logic [63:0] VTYPE_VILL_MASK = 64'h8000000000000000;
parameter string VTYPE_VILL_SW_TYPE = "WARL";

parameter int VTYPE_RESERVED_MSB = 62;
parameter int VTYPE_RESERVED_LSB = 8;
parameter int VTYPE_RESERVED_WIDTH = 55;
parameter logic [54:0] VTYPE_RESERVED_RESET = 64'h0000000000000000[62:8];
parameter logic [63:0] VTYPE_RESERVED_MASK = 64'h7FFFFFFFFFFFFF00;
parameter string VTYPE_RESERVED_SW_TYPE = "WARL";

parameter int VTYPE_VMA_MSB = 7;
parameter int VTYPE_VMA_LSB = 7;
parameter int VTYPE_VMA_WIDTH = 1;
parameter logic [0:0] VTYPE_VMA_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] VTYPE_VMA_MASK = 64'h0000000000000080;
parameter string VTYPE_VMA_SW_TYPE = "WARL";

parameter int VTYPE_VTA_MSB = 6;
parameter int VTYPE_VTA_LSB = 6;
parameter int VTYPE_VTA_WIDTH = 1;
parameter logic [0:0] VTYPE_VTA_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] VTYPE_VTA_MASK = 64'h0000000000000040;
parameter string VTYPE_VTA_SW_TYPE = "WARL";

parameter int VTYPE_VSEW_MSB = 5;
parameter int VTYPE_VSEW_LSB = 3;
parameter int VTYPE_VSEW_WIDTH = 3;
parameter logic [2:0] VTYPE_VSEW_RESET = 64'h0000000000000000[5:3];
parameter logic [63:0] VTYPE_VSEW_MASK = 64'h0000000000000038;
parameter string VTYPE_VSEW_SW_TYPE = "WARL";

parameter int VTYPE_VLMUL_MSB = 2;
parameter int VTYPE_VLMUL_LSB = 0;
parameter int VTYPE_VLMUL_WIDTH = 3;
parameter logic [2:0] VTYPE_VLMUL_RESET = 64'h0000000000000000[2:0];
parameter logic [63:0] VTYPE_VLMUL_MASK = 64'h0000000000000007;
parameter string VTYPE_VLMUL_SW_TYPE = "WARL";


// VLENB CSR Field Defines
parameter int VLENB_VLENB_MSB = 63;
parameter int VLENB_VLENB_LSB = 0;
parameter int VLENB_VLENB_WIDTH = 64;
parameter logic [63:0] VLENB_VLENB_RESET = 64'h0000000000000020[63:0];
parameter logic [63:0] VLENB_VLENB_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VLENB_VLENB_SW_TYPE = "WARL";


// PMPCFG0 CSR Field Defines
parameter int PMPCFG0_PMP7CFG_LOCKED_MSB = 63;
parameter int PMPCFG0_PMP7CFG_LOCKED_LSB = 63;
parameter int PMPCFG0_PMP7CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP7CFG_LOCKED_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] PMPCFG0_PMP7CFG_LOCKED_MASK = 64'h8000000000000000;
parameter string PMPCFG0_PMP7CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP7CFG_RSVD_MSB = 62;
parameter int PMPCFG0_PMP7CFG_RSVD_LSB = 61;
parameter int PMPCFG0_PMP7CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP7CFG_RSVD_RESET = 64'h0000000000000000[62:61];
parameter logic [63:0] PMPCFG0_PMP7CFG_RSVD_MASK = 64'h6000000000000000;
parameter string PMPCFG0_PMP7CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP7CFG_MODE_MSB = 60;
parameter int PMPCFG0_PMP7CFG_MODE_LSB = 59;
parameter int PMPCFG0_PMP7CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP7CFG_MODE_RESET = 64'h0000000000000000[60:59];
parameter logic [63:0] PMPCFG0_PMP7CFG_MODE_MASK = 64'h1800000000000000;
parameter string PMPCFG0_PMP7CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP7CFG_RWX_MSB = 58;
parameter int PMPCFG0_PMP7CFG_RWX_LSB = 56;
parameter int PMPCFG0_PMP7CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP7CFG_RWX_RESET = 64'h0000000000000000[58:56];
parameter logic [63:0] PMPCFG0_PMP7CFG_RWX_MASK = 64'h0700000000000000;
parameter string PMPCFG0_PMP7CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP6CFG_LOCKED_MSB = 55;
parameter int PMPCFG0_PMP6CFG_LOCKED_LSB = 55;
parameter int PMPCFG0_PMP6CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP6CFG_LOCKED_RESET = 64'h0000000000000000[55:55];
parameter logic [63:0] PMPCFG0_PMP6CFG_LOCKED_MASK = 64'h0080000000000000;
parameter string PMPCFG0_PMP6CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP6CFG_RSVD_MSB = 54;
parameter int PMPCFG0_PMP6CFG_RSVD_LSB = 53;
parameter int PMPCFG0_PMP6CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP6CFG_RSVD_RESET = 64'h0000000000000000[54:53];
parameter logic [63:0] PMPCFG0_PMP6CFG_RSVD_MASK = 64'h0060000000000000;
parameter string PMPCFG0_PMP6CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP6CFG_MODE_MSB = 52;
parameter int PMPCFG0_PMP6CFG_MODE_LSB = 51;
parameter int PMPCFG0_PMP6CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP6CFG_MODE_RESET = 64'h0000000000000000[52:51];
parameter logic [63:0] PMPCFG0_PMP6CFG_MODE_MASK = 64'h0018000000000000;
parameter string PMPCFG0_PMP6CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP6CFG_RWX_MSB = 50;
parameter int PMPCFG0_PMP6CFG_RWX_LSB = 48;
parameter int PMPCFG0_PMP6CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP6CFG_RWX_RESET = 64'h0000000000000000[50:48];
parameter logic [63:0] PMPCFG0_PMP6CFG_RWX_MASK = 64'h0007000000000000;
parameter string PMPCFG0_PMP6CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP5CFG_LOCKED_MSB = 47;
parameter int PMPCFG0_PMP5CFG_LOCKED_LSB = 47;
parameter int PMPCFG0_PMP5CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP5CFG_LOCKED_RESET = 64'h0000000000000000[47:47];
parameter logic [63:0] PMPCFG0_PMP5CFG_LOCKED_MASK = 64'h0000800000000000;
parameter string PMPCFG0_PMP5CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP5CFG_RSVD_MSB = 46;
parameter int PMPCFG0_PMP5CFG_RSVD_LSB = 45;
parameter int PMPCFG0_PMP5CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP5CFG_RSVD_RESET = 64'h0000000000000000[46:45];
parameter logic [63:0] PMPCFG0_PMP5CFG_RSVD_MASK = 64'h0000600000000000;
parameter string PMPCFG0_PMP5CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP5CFG_MODE_MSB = 44;
parameter int PMPCFG0_PMP5CFG_MODE_LSB = 43;
parameter int PMPCFG0_PMP5CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP5CFG_MODE_RESET = 64'h0000000000000000[44:43];
parameter logic [63:0] PMPCFG0_PMP5CFG_MODE_MASK = 64'h0000180000000000;
parameter string PMPCFG0_PMP5CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP5CFG_RWX_MSB = 42;
parameter int PMPCFG0_PMP5CFG_RWX_LSB = 40;
parameter int PMPCFG0_PMP5CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP5CFG_RWX_RESET = 64'h0000000000000000[42:40];
parameter logic [63:0] PMPCFG0_PMP5CFG_RWX_MASK = 64'h0000070000000000;
parameter string PMPCFG0_PMP5CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP4CFG_LOCKED_MSB = 39;
parameter int PMPCFG0_PMP4CFG_LOCKED_LSB = 39;
parameter int PMPCFG0_PMP4CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP4CFG_LOCKED_RESET = 64'h0000000000000000[39:39];
parameter logic [63:0] PMPCFG0_PMP4CFG_LOCKED_MASK = 64'h0000008000000000;
parameter string PMPCFG0_PMP4CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP4CFG_RSVD_MSB = 38;
parameter int PMPCFG0_PMP4CFG_RSVD_LSB = 37;
parameter int PMPCFG0_PMP4CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP4CFG_RSVD_RESET = 64'h0000000000000000[38:37];
parameter logic [63:0] PMPCFG0_PMP4CFG_RSVD_MASK = 64'h0000006000000000;
parameter string PMPCFG0_PMP4CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP4CFG_MODE_MSB = 36;
parameter int PMPCFG0_PMP4CFG_MODE_LSB = 35;
parameter int PMPCFG0_PMP4CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP4CFG_MODE_RESET = 64'h0000000000000000[36:35];
parameter logic [63:0] PMPCFG0_PMP4CFG_MODE_MASK = 64'h0000001800000000;
parameter string PMPCFG0_PMP4CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP4CFG_RWX_MSB = 34;
parameter int PMPCFG0_PMP4CFG_RWX_LSB = 32;
parameter int PMPCFG0_PMP4CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP4CFG_RWX_RESET = 64'h0000000000000000[34:32];
parameter logic [63:0] PMPCFG0_PMP4CFG_RWX_MASK = 64'h0000000700000000;
parameter string PMPCFG0_PMP4CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP3CFG_LOCKED_MSB = 31;
parameter int PMPCFG0_PMP3CFG_LOCKED_LSB = 31;
parameter int PMPCFG0_PMP3CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP3CFG_LOCKED_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] PMPCFG0_PMP3CFG_LOCKED_MASK = 64'h0000000080000000;
parameter string PMPCFG0_PMP3CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP3CFG_RSVD_MSB = 30;
parameter int PMPCFG0_PMP3CFG_RSVD_LSB = 29;
parameter int PMPCFG0_PMP3CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP3CFG_RSVD_RESET = 64'h0000000000000000[30:29];
parameter logic [63:0] PMPCFG0_PMP3CFG_RSVD_MASK = 64'h0000000060000000;
parameter string PMPCFG0_PMP3CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP3CFG_MODE_MSB = 28;
parameter int PMPCFG0_PMP3CFG_MODE_LSB = 27;
parameter int PMPCFG0_PMP3CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP3CFG_MODE_RESET = 64'h0000000000000000[28:27];
parameter logic [63:0] PMPCFG0_PMP3CFG_MODE_MASK = 64'h0000000018000000;
parameter string PMPCFG0_PMP3CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP3CFG_RWX_MSB = 26;
parameter int PMPCFG0_PMP3CFG_RWX_LSB = 24;
parameter int PMPCFG0_PMP3CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP3CFG_RWX_RESET = 64'h0000000000000000[26:24];
parameter logic [63:0] PMPCFG0_PMP3CFG_RWX_MASK = 64'h0000000007000000;
parameter string PMPCFG0_PMP3CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP2CFG_LOCKED_MSB = 23;
parameter int PMPCFG0_PMP2CFG_LOCKED_LSB = 23;
parameter int PMPCFG0_PMP2CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP2CFG_LOCKED_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] PMPCFG0_PMP2CFG_LOCKED_MASK = 64'h0000000000800000;
parameter string PMPCFG0_PMP2CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP2CFG_RSVD_MSB = 22;
parameter int PMPCFG0_PMP2CFG_RSVD_LSB = 21;
parameter int PMPCFG0_PMP2CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP2CFG_RSVD_RESET = 64'h0000000000000000[22:21];
parameter logic [63:0] PMPCFG0_PMP2CFG_RSVD_MASK = 64'h0000000000600000;
parameter string PMPCFG0_PMP2CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP2CFG_MODE_MSB = 20;
parameter int PMPCFG0_PMP2CFG_MODE_LSB = 19;
parameter int PMPCFG0_PMP2CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP2CFG_MODE_RESET = 64'h0000000000000000[20:19];
parameter logic [63:0] PMPCFG0_PMP2CFG_MODE_MASK = 64'h0000000000180000;
parameter string PMPCFG0_PMP2CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP2CFG_RWX_MSB = 18;
parameter int PMPCFG0_PMP2CFG_RWX_LSB = 16;
parameter int PMPCFG0_PMP2CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP2CFG_RWX_RESET = 64'h0000000000000000[18:16];
parameter logic [63:0] PMPCFG0_PMP2CFG_RWX_MASK = 64'h0000000000070000;
parameter string PMPCFG0_PMP2CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP1CFG_LOCKED_MSB = 15;
parameter int PMPCFG0_PMP1CFG_LOCKED_LSB = 15;
parameter int PMPCFG0_PMP1CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP1CFG_LOCKED_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] PMPCFG0_PMP1CFG_LOCKED_MASK = 64'h0000000000008000;
parameter string PMPCFG0_PMP1CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP1CFG_RSVD_MSB = 14;
parameter int PMPCFG0_PMP1CFG_RSVD_LSB = 13;
parameter int PMPCFG0_PMP1CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP1CFG_RSVD_RESET = 64'h0000000000000000[14:13];
parameter logic [63:0] PMPCFG0_PMP1CFG_RSVD_MASK = 64'h0000000000006000;
parameter string PMPCFG0_PMP1CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP1CFG_MODE_MSB = 12;
parameter int PMPCFG0_PMP1CFG_MODE_LSB = 11;
parameter int PMPCFG0_PMP1CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP1CFG_MODE_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] PMPCFG0_PMP1CFG_MODE_MASK = 64'h0000000000001800;
parameter string PMPCFG0_PMP1CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP1CFG_RWX_MSB = 10;
parameter int PMPCFG0_PMP1CFG_RWX_LSB = 8;
parameter int PMPCFG0_PMP1CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP1CFG_RWX_RESET = 64'h0000000000000000[10:8];
parameter logic [63:0] PMPCFG0_PMP1CFG_RWX_MASK = 64'h0000000000000700;
parameter string PMPCFG0_PMP1CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP0CFG_LOCKED_MSB = 7;
parameter int PMPCFG0_PMP0CFG_LOCKED_LSB = 7;
parameter int PMPCFG0_PMP0CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG0_PMP0CFG_LOCKED_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] PMPCFG0_PMP0CFG_LOCKED_MASK = 64'h0000000000000080;
parameter string PMPCFG0_PMP0CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP0CFG_RSVD_MSB = 6;
parameter int PMPCFG0_PMP0CFG_RSVD_LSB = 5;
parameter int PMPCFG0_PMP0CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP0CFG_RSVD_RESET = 64'h0000000000000000[6:5];
parameter logic [63:0] PMPCFG0_PMP0CFG_RSVD_MASK = 64'h0000000000000060;
parameter string PMPCFG0_PMP0CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP0CFG_MODE_MSB = 4;
parameter int PMPCFG0_PMP0CFG_MODE_LSB = 3;
parameter int PMPCFG0_PMP0CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG0_PMP0CFG_MODE_RESET = 64'h0000000000000000[4:3];
parameter logic [63:0] PMPCFG0_PMP0CFG_MODE_MASK = 64'h0000000000000018;
parameter string PMPCFG0_PMP0CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG0_PMP0CFG_RWX_MSB = 2;
parameter int PMPCFG0_PMP0CFG_RWX_LSB = 0;
parameter int PMPCFG0_PMP0CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG0_PMP0CFG_RWX_RESET = 64'h0000000000000000[2:0];
parameter logic [63:0] PMPCFG0_PMP0CFG_RWX_MASK = 64'h0000000000000007;
parameter string PMPCFG0_PMP0CFG_RWX_SW_TYPE = "WARL";


// PMPCFG2 CSR Field Defines
parameter int PMPCFG2_PMP15CFG_LOCKED_MSB = 63;
parameter int PMPCFG2_PMP15CFG_LOCKED_LSB = 63;
parameter int PMPCFG2_PMP15CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP15CFG_LOCKED_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] PMPCFG2_PMP15CFG_LOCKED_MASK = 64'h8000000000000000;
parameter string PMPCFG2_PMP15CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP15CFG_RSVD_MSB = 62;
parameter int PMPCFG2_PMP15CFG_RSVD_LSB = 61;
parameter int PMPCFG2_PMP15CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP15CFG_RSVD_RESET = 64'h0000000000000000[62:61];
parameter logic [63:0] PMPCFG2_PMP15CFG_RSVD_MASK = 64'h6000000000000000;
parameter string PMPCFG2_PMP15CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP15CFG_MODE_MSB = 60;
parameter int PMPCFG2_PMP15CFG_MODE_LSB = 59;
parameter int PMPCFG2_PMP15CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP15CFG_MODE_RESET = 64'h0000000000000000[60:59];
parameter logic [63:0] PMPCFG2_PMP15CFG_MODE_MASK = 64'h1800000000000000;
parameter string PMPCFG2_PMP15CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP15CFG_RWX_MSB = 58;
parameter int PMPCFG2_PMP15CFG_RWX_LSB = 56;
parameter int PMPCFG2_PMP15CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP15CFG_RWX_RESET = 64'h0000000000000000[58:56];
parameter logic [63:0] PMPCFG2_PMP15CFG_RWX_MASK = 64'h0700000000000000;
parameter string PMPCFG2_PMP15CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP14CFG_LOCKED_MSB = 55;
parameter int PMPCFG2_PMP14CFG_LOCKED_LSB = 55;
parameter int PMPCFG2_PMP14CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP14CFG_LOCKED_RESET = 64'h0000000000000000[55:55];
parameter logic [63:0] PMPCFG2_PMP14CFG_LOCKED_MASK = 64'h0080000000000000;
parameter string PMPCFG2_PMP14CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP14CFG_RSVD_MSB = 54;
parameter int PMPCFG2_PMP14CFG_RSVD_LSB = 53;
parameter int PMPCFG2_PMP14CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP14CFG_RSVD_RESET = 64'h0000000000000000[54:53];
parameter logic [63:0] PMPCFG2_PMP14CFG_RSVD_MASK = 64'h0060000000000000;
parameter string PMPCFG2_PMP14CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP14CFG_MODE_MSB = 52;
parameter int PMPCFG2_PMP14CFG_MODE_LSB = 51;
parameter int PMPCFG2_PMP14CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP14CFG_MODE_RESET = 64'h0000000000000000[52:51];
parameter logic [63:0] PMPCFG2_PMP14CFG_MODE_MASK = 64'h0018000000000000;
parameter string PMPCFG2_PMP14CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP14CFG_RWX_MSB = 50;
parameter int PMPCFG2_PMP14CFG_RWX_LSB = 48;
parameter int PMPCFG2_PMP14CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP14CFG_RWX_RESET = 64'h0000000000000000[50:48];
parameter logic [63:0] PMPCFG2_PMP14CFG_RWX_MASK = 64'h0007000000000000;
parameter string PMPCFG2_PMP14CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP13CFG_LOCKED_MSB = 47;
parameter int PMPCFG2_PMP13CFG_LOCKED_LSB = 47;
parameter int PMPCFG2_PMP13CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP13CFG_LOCKED_RESET = 64'h0000000000000000[47:47];
parameter logic [63:0] PMPCFG2_PMP13CFG_LOCKED_MASK = 64'h0000800000000000;
parameter string PMPCFG2_PMP13CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP13CFG_RSVD_MSB = 46;
parameter int PMPCFG2_PMP13CFG_RSVD_LSB = 45;
parameter int PMPCFG2_PMP13CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP13CFG_RSVD_RESET = 64'h0000000000000000[46:45];
parameter logic [63:0] PMPCFG2_PMP13CFG_RSVD_MASK = 64'h0000600000000000;
parameter string PMPCFG2_PMP13CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP13CFG_MODE_MSB = 44;
parameter int PMPCFG2_PMP13CFG_MODE_LSB = 43;
parameter int PMPCFG2_PMP13CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP13CFG_MODE_RESET = 64'h0000000000000000[44:43];
parameter logic [63:0] PMPCFG2_PMP13CFG_MODE_MASK = 64'h0000180000000000;
parameter string PMPCFG2_PMP13CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP13CFG_RWX_MSB = 42;
parameter int PMPCFG2_PMP13CFG_RWX_LSB = 40;
parameter int PMPCFG2_PMP13CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP13CFG_RWX_RESET = 64'h0000000000000000[42:40];
parameter logic [63:0] PMPCFG2_PMP13CFG_RWX_MASK = 64'h0000070000000000;
parameter string PMPCFG2_PMP13CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP12CFG_LOCKED_MSB = 39;
parameter int PMPCFG2_PMP12CFG_LOCKED_LSB = 39;
parameter int PMPCFG2_PMP12CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP12CFG_LOCKED_RESET = 64'h0000000000000000[39:39];
parameter logic [63:0] PMPCFG2_PMP12CFG_LOCKED_MASK = 64'h0000008000000000;
parameter string PMPCFG2_PMP12CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP12CFG_RSVD_MSB = 38;
parameter int PMPCFG2_PMP12CFG_RSVD_LSB = 37;
parameter int PMPCFG2_PMP12CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP12CFG_RSVD_RESET = 64'h0000000000000000[38:37];
parameter logic [63:0] PMPCFG2_PMP12CFG_RSVD_MASK = 64'h0000006000000000;
parameter string PMPCFG2_PMP12CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP12CFG_MODE_MSB = 36;
parameter int PMPCFG2_PMP12CFG_MODE_LSB = 35;
parameter int PMPCFG2_PMP12CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP12CFG_MODE_RESET = 64'h0000000000000000[36:35];
parameter logic [63:0] PMPCFG2_PMP12CFG_MODE_MASK = 64'h0000001800000000;
parameter string PMPCFG2_PMP12CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP12CFG_RWX_MSB = 34;
parameter int PMPCFG2_PMP12CFG_RWX_LSB = 32;
parameter int PMPCFG2_PMP12CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP12CFG_RWX_RESET = 64'h0000000000000000[34:32];
parameter logic [63:0] PMPCFG2_PMP12CFG_RWX_MASK = 64'h0000000700000000;
parameter string PMPCFG2_PMP12CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP11CFG_LOCKED_MSB = 31;
parameter int PMPCFG2_PMP11CFG_LOCKED_LSB = 31;
parameter int PMPCFG2_PMP11CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP11CFG_LOCKED_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] PMPCFG2_PMP11CFG_LOCKED_MASK = 64'h0000000080000000;
parameter string PMPCFG2_PMP11CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP11CFG_RSVD_MSB = 30;
parameter int PMPCFG2_PMP11CFG_RSVD_LSB = 29;
parameter int PMPCFG2_PMP11CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP11CFG_RSVD_RESET = 64'h0000000000000000[30:29];
parameter logic [63:0] PMPCFG2_PMP11CFG_RSVD_MASK = 64'h0000000060000000;
parameter string PMPCFG2_PMP11CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP11CFG_MODE_MSB = 28;
parameter int PMPCFG2_PMP11CFG_MODE_LSB = 27;
parameter int PMPCFG2_PMP11CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP11CFG_MODE_RESET = 64'h0000000000000000[28:27];
parameter logic [63:0] PMPCFG2_PMP11CFG_MODE_MASK = 64'h0000000018000000;
parameter string PMPCFG2_PMP11CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP11CFG_RWX_MSB = 26;
parameter int PMPCFG2_PMP11CFG_RWX_LSB = 24;
parameter int PMPCFG2_PMP11CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP11CFG_RWX_RESET = 64'h0000000000000000[26:24];
parameter logic [63:0] PMPCFG2_PMP11CFG_RWX_MASK = 64'h0000000007000000;
parameter string PMPCFG2_PMP11CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP10CFG_LOCKED_MSB = 23;
parameter int PMPCFG2_PMP10CFG_LOCKED_LSB = 23;
parameter int PMPCFG2_PMP10CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP10CFG_LOCKED_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] PMPCFG2_PMP10CFG_LOCKED_MASK = 64'h0000000000800000;
parameter string PMPCFG2_PMP10CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP10CFG_RSVD_MSB = 22;
parameter int PMPCFG2_PMP10CFG_RSVD_LSB = 21;
parameter int PMPCFG2_PMP10CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP10CFG_RSVD_RESET = 64'h0000000000000000[22:21];
parameter logic [63:0] PMPCFG2_PMP10CFG_RSVD_MASK = 64'h0000000000600000;
parameter string PMPCFG2_PMP10CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP10CFG_MODE_MSB = 20;
parameter int PMPCFG2_PMP10CFG_MODE_LSB = 19;
parameter int PMPCFG2_PMP10CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP10CFG_MODE_RESET = 64'h0000000000000000[20:19];
parameter logic [63:0] PMPCFG2_PMP10CFG_MODE_MASK = 64'h0000000000180000;
parameter string PMPCFG2_PMP10CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP10CFG_RWX_MSB = 18;
parameter int PMPCFG2_PMP10CFG_RWX_LSB = 16;
parameter int PMPCFG2_PMP10CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP10CFG_RWX_RESET = 64'h0000000000000000[18:16];
parameter logic [63:0] PMPCFG2_PMP10CFG_RWX_MASK = 64'h0000000000070000;
parameter string PMPCFG2_PMP10CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP9CFG_LOCKED_MSB = 15;
parameter int PMPCFG2_PMP9CFG_LOCKED_LSB = 15;
parameter int PMPCFG2_PMP9CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP9CFG_LOCKED_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] PMPCFG2_PMP9CFG_LOCKED_MASK = 64'h0000000000008000;
parameter string PMPCFG2_PMP9CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP9CFG_RSVD_MSB = 14;
parameter int PMPCFG2_PMP9CFG_RSVD_LSB = 13;
parameter int PMPCFG2_PMP9CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP9CFG_RSVD_RESET = 64'h0000000000000000[14:13];
parameter logic [63:0] PMPCFG2_PMP9CFG_RSVD_MASK = 64'h0000000000006000;
parameter string PMPCFG2_PMP9CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP9CFG_MODE_MSB = 12;
parameter int PMPCFG2_PMP9CFG_MODE_LSB = 11;
parameter int PMPCFG2_PMP9CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP9CFG_MODE_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] PMPCFG2_PMP9CFG_MODE_MASK = 64'h0000000000001800;
parameter string PMPCFG2_PMP9CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP9CFG_RWX_MSB = 10;
parameter int PMPCFG2_PMP9CFG_RWX_LSB = 8;
parameter int PMPCFG2_PMP9CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP9CFG_RWX_RESET = 64'h0000000000000000[10:8];
parameter logic [63:0] PMPCFG2_PMP9CFG_RWX_MASK = 64'h0000000000000700;
parameter string PMPCFG2_PMP9CFG_RWX_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP8CFG_LOCKED_MSB = 7;
parameter int PMPCFG2_PMP8CFG_LOCKED_LSB = 7;
parameter int PMPCFG2_PMP8CFG_LOCKED_WIDTH = 1;
parameter logic [0:0] PMPCFG2_PMP8CFG_LOCKED_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] PMPCFG2_PMP8CFG_LOCKED_MASK = 64'h0000000000000080;
parameter string PMPCFG2_PMP8CFG_LOCKED_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP8CFG_RSVD_MSB = 6;
parameter int PMPCFG2_PMP8CFG_RSVD_LSB = 5;
parameter int PMPCFG2_PMP8CFG_RSVD_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP8CFG_RSVD_RESET = 64'h0000000000000000[6:5];
parameter logic [63:0] PMPCFG2_PMP8CFG_RSVD_MASK = 64'h0000000000000060;
parameter string PMPCFG2_PMP8CFG_RSVD_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP8CFG_MODE_MSB = 4;
parameter int PMPCFG2_PMP8CFG_MODE_LSB = 3;
parameter int PMPCFG2_PMP8CFG_MODE_WIDTH = 2;
parameter logic [1:0] PMPCFG2_PMP8CFG_MODE_RESET = 64'h0000000000000000[4:3];
parameter logic [63:0] PMPCFG2_PMP8CFG_MODE_MASK = 64'h0000000000000018;
parameter string PMPCFG2_PMP8CFG_MODE_SW_TYPE = "WARL";

parameter int PMPCFG2_PMP8CFG_RWX_MSB = 2;
parameter int PMPCFG2_PMP8CFG_RWX_LSB = 0;
parameter int PMPCFG2_PMP8CFG_RWX_WIDTH = 3;
parameter logic [2:0] PMPCFG2_PMP8CFG_RWX_RESET = 64'h0000000000000000[2:0];
parameter logic [63:0] PMPCFG2_PMP8CFG_RWX_MASK = 64'h0000000000000007;
parameter string PMPCFG2_PMP8CFG_RWX_SW_TYPE = "WARL";


// PMPADDR0 CSR Field Defines
parameter int PMPADDR0_ADDRESS_HI_MSB = 53;
parameter int PMPADDR0_ADDRESS_HI_LSB = 9;
parameter int PMPADDR0_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR0_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR0_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR0_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR0_ADDRESS_LO_MSB = 8;
parameter int PMPADDR0_ADDRESS_LO_LSB = 0;
parameter int PMPADDR0_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR0_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR0_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR0_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR1 CSR Field Defines
parameter int PMPADDR1_ADDRESS_HI_MSB = 53;
parameter int PMPADDR1_ADDRESS_HI_LSB = 9;
parameter int PMPADDR1_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR1_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR1_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR1_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR1_ADDRESS_LO_MSB = 8;
parameter int PMPADDR1_ADDRESS_LO_LSB = 0;
parameter int PMPADDR1_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR1_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR1_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR1_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR2 CSR Field Defines
parameter int PMPADDR2_ADDRESS_HI_MSB = 53;
parameter int PMPADDR2_ADDRESS_HI_LSB = 9;
parameter int PMPADDR2_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR2_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR2_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR2_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR2_ADDRESS_LO_MSB = 8;
parameter int PMPADDR2_ADDRESS_LO_LSB = 0;
parameter int PMPADDR2_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR2_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR2_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR2_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR3 CSR Field Defines
parameter int PMPADDR3_ADDRESS_HI_MSB = 53;
parameter int PMPADDR3_ADDRESS_HI_LSB = 9;
parameter int PMPADDR3_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR3_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR3_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR3_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR3_ADDRESS_LO_MSB = 8;
parameter int PMPADDR3_ADDRESS_LO_LSB = 0;
parameter int PMPADDR3_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR3_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR3_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR3_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR4 CSR Field Defines
parameter int PMPADDR4_ADDRESS_HI_MSB = 53;
parameter int PMPADDR4_ADDRESS_HI_LSB = 9;
parameter int PMPADDR4_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR4_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR4_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR4_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR4_ADDRESS_LO_MSB = 8;
parameter int PMPADDR4_ADDRESS_LO_LSB = 0;
parameter int PMPADDR4_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR4_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR4_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR4_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR5 CSR Field Defines
parameter int PMPADDR5_ADDRESS_HI_MSB = 53;
parameter int PMPADDR5_ADDRESS_HI_LSB = 9;
parameter int PMPADDR5_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR5_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR5_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR5_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR5_ADDRESS_LO_MSB = 8;
parameter int PMPADDR5_ADDRESS_LO_LSB = 0;
parameter int PMPADDR5_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR5_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR5_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR5_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR6 CSR Field Defines
parameter int PMPADDR6_ADDRESS_HI_MSB = 53;
parameter int PMPADDR6_ADDRESS_HI_LSB = 9;
parameter int PMPADDR6_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR6_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR6_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR6_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR6_ADDRESS_LO_MSB = 8;
parameter int PMPADDR6_ADDRESS_LO_LSB = 0;
parameter int PMPADDR6_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR6_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR6_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR6_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR7 CSR Field Defines
parameter int PMPADDR7_ADDRESS_HI_MSB = 53;
parameter int PMPADDR7_ADDRESS_HI_LSB = 9;
parameter int PMPADDR7_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR7_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR7_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR7_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR7_ADDRESS_LO_MSB = 8;
parameter int PMPADDR7_ADDRESS_LO_LSB = 0;
parameter int PMPADDR7_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR7_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR7_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR7_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR8 CSR Field Defines
parameter int PMPADDR8_ADDRESS_HI_MSB = 53;
parameter int PMPADDR8_ADDRESS_HI_LSB = 9;
parameter int PMPADDR8_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR8_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR8_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR8_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR8_ADDRESS_LO_MSB = 8;
parameter int PMPADDR8_ADDRESS_LO_LSB = 0;
parameter int PMPADDR8_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR8_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR8_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR8_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR9 CSR Field Defines
parameter int PMPADDR9_ADDRESS_HI_MSB = 53;
parameter int PMPADDR9_ADDRESS_HI_LSB = 9;
parameter int PMPADDR9_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR9_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR9_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR9_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR9_ADDRESS_LO_MSB = 8;
parameter int PMPADDR9_ADDRESS_LO_LSB = 0;
parameter int PMPADDR9_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR9_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR9_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR9_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR10 CSR Field Defines
parameter int PMPADDR10_ADDRESS_HI_MSB = 53;
parameter int PMPADDR10_ADDRESS_HI_LSB = 9;
parameter int PMPADDR10_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR10_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR10_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR10_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR10_ADDRESS_LO_MSB = 8;
parameter int PMPADDR10_ADDRESS_LO_LSB = 0;
parameter int PMPADDR10_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR10_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR10_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR10_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR11 CSR Field Defines
parameter int PMPADDR11_ADDRESS_HI_MSB = 53;
parameter int PMPADDR11_ADDRESS_HI_LSB = 9;
parameter int PMPADDR11_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR11_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR11_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR11_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR11_ADDRESS_LO_MSB = 8;
parameter int PMPADDR11_ADDRESS_LO_LSB = 0;
parameter int PMPADDR11_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR11_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR11_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR11_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR12 CSR Field Defines
parameter int PMPADDR12_ADDRESS_HI_MSB = 53;
parameter int PMPADDR12_ADDRESS_HI_LSB = 9;
parameter int PMPADDR12_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR12_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR12_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR12_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR12_ADDRESS_LO_MSB = 8;
parameter int PMPADDR12_ADDRESS_LO_LSB = 0;
parameter int PMPADDR12_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR12_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR12_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR12_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR13 CSR Field Defines
parameter int PMPADDR13_ADDRESS_HI_MSB = 53;
parameter int PMPADDR13_ADDRESS_HI_LSB = 9;
parameter int PMPADDR13_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR13_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR13_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR13_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR13_ADDRESS_LO_MSB = 8;
parameter int PMPADDR13_ADDRESS_LO_LSB = 0;
parameter int PMPADDR13_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR13_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR13_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR13_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR14 CSR Field Defines
parameter int PMPADDR14_ADDRESS_HI_MSB = 53;
parameter int PMPADDR14_ADDRESS_HI_LSB = 9;
parameter int PMPADDR14_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR14_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR14_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR14_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR14_ADDRESS_LO_MSB = 8;
parameter int PMPADDR14_ADDRESS_LO_LSB = 0;
parameter int PMPADDR14_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR14_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR14_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR14_ADDRESS_LO_SW_TYPE = "WARL";


// PMPADDR15 CSR Field Defines
parameter int PMPADDR15_ADDRESS_HI_MSB = 53;
parameter int PMPADDR15_ADDRESS_HI_LSB = 9;
parameter int PMPADDR15_ADDRESS_HI_WIDTH = 45;
parameter logic [44:0] PMPADDR15_ADDRESS_HI_RESET = 64'h0000000000000000[53:9];
parameter logic [63:0] PMPADDR15_ADDRESS_HI_MASK = 64'h003FFFFFFFFFFE00;
parameter string PMPADDR15_ADDRESS_HI_SW_TYPE = "WARL";

parameter int PMPADDR15_ADDRESS_LO_MSB = 8;
parameter int PMPADDR15_ADDRESS_LO_LSB = 0;
parameter int PMPADDR15_ADDRESS_LO_WIDTH = 9;
parameter logic [8:0] PMPADDR15_ADDRESS_LO_RESET = 64'h00000000000001FF[8:0];
parameter logic [63:0] PMPADDR15_ADDRESS_LO_MASK = 64'h00000000000001FF;
parameter string PMPADDR15_ADDRESS_LO_SW_TYPE = "WARL";


// TSELECT CSR Field Defines
parameter int TSELECT_INDEX_MSB = 63;
parameter int TSELECT_INDEX_LSB = 0;
parameter int TSELECT_INDEX_WIDTH = 64;
parameter logic [63:0] TSELECT_INDEX_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] TSELECT_INDEX_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string TSELECT_INDEX_SW_TYPE = "WARL";


// DCSR CSR Field Defines
parameter int DCSR_XDEBUGVER_MSB = 31;
parameter int DCSR_XDEBUGVER_LSB = 28;
parameter int DCSR_XDEBUGVER_WIDTH = 4;
parameter logic [3:0] DCSR_XDEBUGVER_RESET = 64'h0000000000000000[31:28];
parameter logic [63:0] DCSR_XDEBUGVER_MASK = 64'h00000000F0000000;
parameter string DCSR_XDEBUGVER_SW_TYPE = "WARL";

parameter int DCSR_HARD0_2_MSB = 27;
parameter int DCSR_HARD0_2_LSB = 18;
parameter int DCSR_HARD0_2_WIDTH = 10;
parameter logic [9:0] DCSR_HARD0_2_RESET = 64'h0000000000000000[27:18];
parameter logic [63:0] DCSR_HARD0_2_MASK = 64'h000000000FFC0000;
parameter string DCSR_HARD0_2_SW_TYPE = "WARL";

parameter int DCSR_EBREAKVS_MSB = 17;
parameter int DCSR_EBREAKVS_LSB = 17;
parameter int DCSR_EBREAKVS_WIDTH = 1;
parameter logic [0:0] DCSR_EBREAKVS_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] DCSR_EBREAKVS_MASK = 64'h0000000000020000;
parameter string DCSR_EBREAKVS_SW_TYPE = "WARL";

parameter int DCSR_EBREAKVU_MSB = 16;
parameter int DCSR_EBREAKVU_LSB = 16;
parameter int DCSR_EBREAKVU_WIDTH = 1;
parameter logic [0:0] DCSR_EBREAKVU_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] DCSR_EBREAKVU_MASK = 64'h0000000000010000;
parameter string DCSR_EBREAKVU_SW_TYPE = "WARL";

parameter int DCSR_EBREAKM_MSB = 15;
parameter int DCSR_EBREAKM_LSB = 15;
parameter int DCSR_EBREAKM_WIDTH = 1;
parameter logic [0:0] DCSR_EBREAKM_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] DCSR_EBREAKM_MASK = 64'h0000000000008000;
parameter string DCSR_EBREAKM_SW_TYPE = "WARL";

parameter int DCSR_HARD0_1_MSB = 14;
parameter int DCSR_HARD0_1_LSB = 14;
parameter int DCSR_HARD0_1_WIDTH = 1;
parameter logic [0:0] DCSR_HARD0_1_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] DCSR_HARD0_1_MASK = 64'h0000000000004000;
parameter string DCSR_HARD0_1_SW_TYPE = "WARL";

parameter int DCSR_EBREAKS_MSB = 13;
parameter int DCSR_EBREAKS_LSB = 13;
parameter int DCSR_EBREAKS_WIDTH = 1;
parameter logic [0:0] DCSR_EBREAKS_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] DCSR_EBREAKS_MASK = 64'h0000000000002000;
parameter string DCSR_EBREAKS_SW_TYPE = "WARL";

parameter int DCSR_EBREAKU_MSB = 12;
parameter int DCSR_EBREAKU_LSB = 12;
parameter int DCSR_EBREAKU_WIDTH = 1;
parameter logic [0:0] DCSR_EBREAKU_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] DCSR_EBREAKU_MASK = 64'h0000000000001000;
parameter string DCSR_EBREAKU_SW_TYPE = "WARL";

parameter int DCSR_STEPIE_MSB = 11;
parameter int DCSR_STEPIE_LSB = 11;
parameter int DCSR_STEPIE_WIDTH = 1;
parameter logic [0:0] DCSR_STEPIE_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] DCSR_STEPIE_MASK = 64'h0000000000000800;
parameter string DCSR_STEPIE_SW_TYPE = "WARL";

parameter int DCSR_STOPCOUNT_MSB = 10;
parameter int DCSR_STOPCOUNT_LSB = 10;
parameter int DCSR_STOPCOUNT_WIDTH = 1;
parameter logic [0:0] DCSR_STOPCOUNT_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] DCSR_STOPCOUNT_MASK = 64'h0000000000000400;
parameter string DCSR_STOPCOUNT_SW_TYPE = "WARL";

parameter int DCSR_STOPTIME_MSB = 9;
parameter int DCSR_STOPTIME_LSB = 9;
parameter int DCSR_STOPTIME_WIDTH = 1;
parameter logic [0:0] DCSR_STOPTIME_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] DCSR_STOPTIME_MASK = 64'h0000000000000200;
parameter string DCSR_STOPTIME_SW_TYPE = "WARL";

parameter int DCSR_CAUSE_MSB = 8;
parameter int DCSR_CAUSE_LSB = 6;
parameter int DCSR_CAUSE_WIDTH = 3;
parameter logic [2:0] DCSR_CAUSE_RESET = 64'h0000000000000000[8:6];
parameter logic [63:0] DCSR_CAUSE_MASK = 64'h00000000000001C0;
parameter string DCSR_CAUSE_SW_TYPE = "WARL";

parameter int DCSR_V_MSB = 5;
parameter int DCSR_V_LSB = 5;
parameter int DCSR_V_WIDTH = 1;
parameter logic [0:0] DCSR_V_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] DCSR_V_MASK = 64'h0000000000000020;
parameter string DCSR_V_SW_TYPE = "WARL";

parameter int DCSR_MPRVEN_MSB = 4;
parameter int DCSR_MPRVEN_LSB = 4;
parameter int DCSR_MPRVEN_WIDTH = 1;
parameter logic [0:0] DCSR_MPRVEN_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] DCSR_MPRVEN_MASK = 64'h0000000000000010;
parameter string DCSR_MPRVEN_SW_TYPE = "WARL";

parameter int DCSR_NMIP_MSB = 3;
parameter int DCSR_NMIP_LSB = 3;
parameter int DCSR_NMIP_WIDTH = 1;
parameter logic [0:0] DCSR_NMIP_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] DCSR_NMIP_MASK = 64'h0000000000000008;
parameter string DCSR_NMIP_SW_TYPE = "WARL";

parameter int DCSR_STEP_MSB = 2;
parameter int DCSR_STEP_LSB = 2;
parameter int DCSR_STEP_WIDTH = 1;
parameter logic [0:0] DCSR_STEP_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] DCSR_STEP_MASK = 64'h0000000000000004;
parameter string DCSR_STEP_SW_TYPE = "WARL";

parameter int DCSR_PRV_MSB = 1;
parameter int DCSR_PRV_LSB = 0;
parameter int DCSR_PRV_WIDTH = 2;
parameter logic [1:0] DCSR_PRV_RESET = 64'h0000000000000003[1:0];
parameter logic [63:0] DCSR_PRV_MASK = 64'h0000000000000003;
parameter string DCSR_PRV_SW_TYPE = "WARL";


// DPC CSR Field Defines
parameter int DPC_DPC_MSB = 63;
parameter int DPC_DPC_LSB = 0;
parameter int DPC_DPC_WIDTH = 64;
parameter logic [63:0] DPC_DPC_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] DPC_DPC_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string DPC_DPC_SW_TYPE = "WARL";


// DSCRATCH0 CSR Field Defines
parameter int DSCRATCH0_DSCRATCH0_MSB = 63;
parameter int DSCRATCH0_DSCRATCH0_LSB = 0;
parameter int DSCRATCH0_DSCRATCH0_WIDTH = 64;
parameter logic [63:0] DSCRATCH0_DSCRATCH0_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] DSCRATCH0_DSCRATCH0_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string DSCRATCH0_DSCRATCH0_SW_TYPE = "WARL";


// DSCRATCH1 CSR Field Defines
parameter int DSCRATCH1_DSCRATCH1_MSB = 63;
parameter int DSCRATCH1_DSCRATCH1_LSB = 0;
parameter int DSCRATCH1_DSCRATCH1_WIDTH = 64;
parameter logic [63:0] DSCRATCH1_DSCRATCH1_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] DSCRATCH1_DSCRATCH1_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string DSCRATCH1_DSCRATCH1_SW_TYPE = "WARL";


// HSTATUS CSR Field Defines
parameter int HSTATUS_WPRI_5_MSB = 63;
parameter int HSTATUS_WPRI_5_LSB = 50;
parameter int HSTATUS_WPRI_5_WIDTH = 14;
parameter logic [13:0] HSTATUS_WPRI_5_RESET = 64'h0000000000000000[63:50];
parameter logic [63:0] HSTATUS_WPRI_5_MASK = 64'hFFFC000000000000;
parameter string HSTATUS_WPRI_5_SW_TYPE = "WPRI";

parameter int HSTATUS_HUPMM_MSB = 49;
parameter int HSTATUS_HUPMM_LSB = 48;
parameter int HSTATUS_HUPMM_WIDTH = 2;
parameter logic [1:0] HSTATUS_HUPMM_RESET = 64'h0000000000000000[49:48];
parameter logic [63:0] HSTATUS_HUPMM_MASK = 64'h0003000000000000;
parameter string HSTATUS_HUPMM_SW_TYPE = "WARL";

parameter int HSTATUS_WPRI_4_MSB = 47;
parameter int HSTATUS_WPRI_4_LSB = 34;
parameter int HSTATUS_WPRI_4_WIDTH = 14;
parameter logic [13:0] HSTATUS_WPRI_4_RESET = 64'h0000000000000000[47:34];
parameter logic [63:0] HSTATUS_WPRI_4_MASK = 64'h0000FFFC00000000;
parameter string HSTATUS_WPRI_4_SW_TYPE = "WPRI";

parameter int HSTATUS_VSXL_MSB = 33;
parameter int HSTATUS_VSXL_LSB = 32;
parameter int HSTATUS_VSXL_WIDTH = 2;
parameter logic [1:0] HSTATUS_VSXL_RESET = 64'h0000000000000002[33:32];
parameter logic [63:0] HSTATUS_VSXL_MASK = 64'h0000000300000000;
parameter string HSTATUS_VSXL_SW_TYPE = "WARL";

parameter int HSTATUS_WPRI_3_MSB = 31;
parameter int HSTATUS_WPRI_3_LSB = 23;
parameter int HSTATUS_WPRI_3_WIDTH = 9;
parameter logic [8:0] HSTATUS_WPRI_3_RESET = 64'h0000000000000000[31:23];
parameter logic [63:0] HSTATUS_WPRI_3_MASK = 64'h00000000FF800000;
parameter string HSTATUS_WPRI_3_SW_TYPE = "WPRI";

parameter int HSTATUS_VTSR_MSB = 22;
parameter int HSTATUS_VTSR_LSB = 22;
parameter int HSTATUS_VTSR_WIDTH = 1;
parameter logic [0:0] HSTATUS_VTSR_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] HSTATUS_VTSR_MASK = 64'h0000000000400000;
parameter string HSTATUS_VTSR_SW_TYPE = "WARL";

parameter int HSTATUS_VTW_MSB = 21;
parameter int HSTATUS_VTW_LSB = 21;
parameter int HSTATUS_VTW_WIDTH = 1;
parameter logic [0:0] HSTATUS_VTW_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] HSTATUS_VTW_MASK = 64'h0000000000200000;
parameter string HSTATUS_VTW_SW_TYPE = "WARL";

parameter int HSTATUS_VTVM_MSB = 20;
parameter int HSTATUS_VTVM_LSB = 20;
parameter int HSTATUS_VTVM_WIDTH = 1;
parameter logic [0:0] HSTATUS_VTVM_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] HSTATUS_VTVM_MASK = 64'h0000000000100000;
parameter string HSTATUS_VTVM_SW_TYPE = "WARL";

parameter int HSTATUS_WPRI_2_MSB = 19;
parameter int HSTATUS_WPRI_2_LSB = 18;
parameter int HSTATUS_WPRI_2_WIDTH = 2;
parameter logic [1:0] HSTATUS_WPRI_2_RESET = 64'h0000000000000000[19:18];
parameter logic [63:0] HSTATUS_WPRI_2_MASK = 64'h00000000000C0000;
parameter string HSTATUS_WPRI_2_SW_TYPE = "WPRI";

parameter int HSTATUS_VGEIN_MSB = 17;
parameter int HSTATUS_VGEIN_LSB = 12;
parameter int HSTATUS_VGEIN_WIDTH = 6;
parameter logic [5:0] HSTATUS_VGEIN_RESET = 64'h0000000000000000[17:12];
parameter logic [63:0] HSTATUS_VGEIN_MASK = 64'h000000000003F000;
parameter string HSTATUS_VGEIN_SW_TYPE = "WARL";

parameter int HSTATUS_WPRI_1_MSB = 11;
parameter int HSTATUS_WPRI_1_LSB = 10;
parameter int HSTATUS_WPRI_1_WIDTH = 2;
parameter logic [1:0] HSTATUS_WPRI_1_RESET = 64'h0000000000000000[11:10];
parameter logic [63:0] HSTATUS_WPRI_1_MASK = 64'h0000000000000C00;
parameter string HSTATUS_WPRI_1_SW_TYPE = "WPRI";

parameter int HSTATUS_HU_MSB = 9;
parameter int HSTATUS_HU_LSB = 9;
parameter int HSTATUS_HU_WIDTH = 1;
parameter logic [0:0] HSTATUS_HU_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HSTATUS_HU_MASK = 64'h0000000000000200;
parameter string HSTATUS_HU_SW_TYPE = "WARL";

parameter int HSTATUS_SPVP_MSB = 8;
parameter int HSTATUS_SPVP_LSB = 8;
parameter int HSTATUS_SPVP_WIDTH = 1;
parameter logic [0:0] HSTATUS_SPVP_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] HSTATUS_SPVP_MASK = 64'h0000000000000100;
parameter string HSTATUS_SPVP_SW_TYPE = "WARL";

parameter int HSTATUS_SPV_MSB = 7;
parameter int HSTATUS_SPV_LSB = 7;
parameter int HSTATUS_SPV_WIDTH = 1;
parameter logic [0:0] HSTATUS_SPV_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HSTATUS_SPV_MASK = 64'h0000000000000080;
parameter string HSTATUS_SPV_SW_TYPE = "WARL";

parameter int HSTATUS_GVA_MSB = 6;
parameter int HSTATUS_GVA_LSB = 6;
parameter int HSTATUS_GVA_WIDTH = 1;
parameter logic [0:0] HSTATUS_GVA_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HSTATUS_GVA_MASK = 64'h0000000000000040;
parameter string HSTATUS_GVA_SW_TYPE = "WARL";

parameter int HSTATUS_VSBE_MSB = 5;
parameter int HSTATUS_VSBE_LSB = 5;
parameter int HSTATUS_VSBE_WIDTH = 1;
parameter logic [0:0] HSTATUS_VSBE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HSTATUS_VSBE_MASK = 64'h0000000000000020;
parameter string HSTATUS_VSBE_SW_TYPE = "WARL";

parameter int HSTATUS_WPRI_0_MSB = 4;
parameter int HSTATUS_WPRI_0_LSB = 0;
parameter int HSTATUS_WPRI_0_WIDTH = 5;
parameter logic [4:0] HSTATUS_WPRI_0_RESET = 64'h0000000000000000[4:0];
parameter logic [63:0] HSTATUS_WPRI_0_MASK = 64'h000000000000001F;
parameter string HSTATUS_WPRI_0_SW_TYPE = "WPRI";


// HEDELEG CSR Field Defines
parameter int HEDELEG_HARD0_3_MSB = 63;
parameter int HEDELEG_HARD0_3_LSB = 24;
parameter int HEDELEG_HARD0_3_WIDTH = 40;
parameter logic [39:0] HEDELEG_HARD0_3_RESET = 64'h0000000000000000[63:24];
parameter logic [63:0] HEDELEG_HARD0_3_MASK = 64'hFFFFFFFFFF000000;
parameter string HEDELEG_HARD0_3_SW_TYPE = "WARL";

parameter int HEDELEG_HARD0_2_MSB = 23;
parameter int HEDELEG_HARD0_2_LSB = 20;
parameter int HEDELEG_HARD0_2_WIDTH = 4;
parameter logic [3:0] HEDELEG_HARD0_2_RESET = 64'h0000000000000000[23:20];
parameter logic [63:0] HEDELEG_HARD0_2_MASK = 64'h0000000000F00000;
parameter string HEDELEG_HARD0_2_SW_TYPE = "WARL";

parameter int HEDELEG_HEDELEG_3_MSB = 19;
parameter int HEDELEG_HEDELEG_3_LSB = 16;
parameter int HEDELEG_HEDELEG_3_WIDTH = 4;
parameter logic [3:0] HEDELEG_HEDELEG_3_RESET = 64'h0000000000000000[19:16];
parameter logic [63:0] HEDELEG_HEDELEG_3_MASK = 64'h00000000000F0000;
parameter string HEDELEG_HEDELEG_3_SW_TYPE = "WARL";

parameter int HEDELEG_HEDELEG_2_MSB = 15;
parameter int HEDELEG_HEDELEG_2_LSB = 15;
parameter int HEDELEG_HEDELEG_2_WIDTH = 1;
parameter logic [0:0] HEDELEG_HEDELEG_2_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] HEDELEG_HEDELEG_2_MASK = 64'h0000000000008000;
parameter string HEDELEG_HEDELEG_2_SW_TYPE = "WARL";

parameter int HEDELEG_HARD0_1_MSB = 14;
parameter int HEDELEG_HARD0_1_LSB = 14;
parameter int HEDELEG_HARD0_1_WIDTH = 1;
parameter logic [0:0] HEDELEG_HARD0_1_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] HEDELEG_HARD0_1_MASK = 64'h0000000000004000;
parameter string HEDELEG_HARD0_1_SW_TYPE = "WARL";

parameter int HEDELEG_HEDELEG_1_MSB = 13;
parameter int HEDELEG_HEDELEG_1_LSB = 12;
parameter int HEDELEG_HEDELEG_1_WIDTH = 2;
parameter logic [1:0] HEDELEG_HEDELEG_1_RESET = 64'h0000000000000000[13:12];
parameter logic [63:0] HEDELEG_HEDELEG_1_MASK = 64'h0000000000003000;
parameter string HEDELEG_HEDELEG_1_SW_TYPE = "WARL";

parameter int HEDELEG_HARD0_0_MSB = 11;
parameter int HEDELEG_HARD0_0_LSB = 9;
parameter int HEDELEG_HARD0_0_WIDTH = 3;
parameter logic [2:0] HEDELEG_HARD0_0_RESET = 64'h0000000000000000[11:9];
parameter logic [63:0] HEDELEG_HARD0_0_MASK = 64'h0000000000000E00;
parameter string HEDELEG_HARD0_0_SW_TYPE = "WARL";

parameter int HEDELEG_HEDELEG_0_MSB = 8;
parameter int HEDELEG_HEDELEG_0_LSB = 0;
parameter int HEDELEG_HEDELEG_0_WIDTH = 9;
parameter logic [8:0] HEDELEG_HEDELEG_0_RESET = 64'h0000000000000000[8:0];
parameter logic [63:0] HEDELEG_HEDELEG_0_MASK = 64'h00000000000001FF;
parameter string HEDELEG_HEDELEG_0_SW_TYPE = "WARL";


// HIDELEG CSR Field Defines
parameter int HIDELEG_LCOFIP_MSB = 13;
parameter int HIDELEG_LCOFIP_LSB = 13;
parameter int HIDELEG_LCOFIP_WIDTH = 1;
parameter logic [0:0] HIDELEG_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] HIDELEG_LCOFIP_MASK = 64'h0000000000002000;
parameter string HIDELEG_LCOFIP_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_5_MSB = 12;
parameter int HIDELEG_HARD0_5_LSB = 11;
parameter int HIDELEG_HARD0_5_WIDTH = 2;
parameter logic [1:0] HIDELEG_HARD0_5_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] HIDELEG_HARD0_5_MASK = 64'h0000000000001800;
parameter string HIDELEG_HARD0_5_SW_TYPE = "WARL";

parameter int HIDELEG_VSEIP_MSB = 10;
parameter int HIDELEG_VSEIP_LSB = 10;
parameter int HIDELEG_VSEIP_WIDTH = 1;
parameter logic [0:0] HIDELEG_VSEIP_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] HIDELEG_VSEIP_MASK = 64'h0000000000000400;
parameter string HIDELEG_VSEIP_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_4_MSB = 9;
parameter int HIDELEG_HARD0_4_LSB = 9;
parameter int HIDELEG_HARD0_4_WIDTH = 1;
parameter logic [0:0] HIDELEG_HARD0_4_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HIDELEG_HARD0_4_MASK = 64'h0000000000000200;
parameter string HIDELEG_HARD0_4_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_3_MSB = 7;
parameter int HIDELEG_HARD0_3_LSB = 7;
parameter int HIDELEG_HARD0_3_WIDTH = 1;
parameter logic [0:0] HIDELEG_HARD0_3_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HIDELEG_HARD0_3_MASK = 64'h0000000000000080;
parameter string HIDELEG_HARD0_3_SW_TYPE = "WARL";

parameter int HIDELEG_VSTIP_MSB = 6;
parameter int HIDELEG_VSTIP_LSB = 6;
parameter int HIDELEG_VSTIP_WIDTH = 1;
parameter logic [0:0] HIDELEG_VSTIP_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HIDELEG_VSTIP_MASK = 64'h0000000000000040;
parameter string HIDELEG_VSTIP_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_2_MSB = 5;
parameter int HIDELEG_HARD0_2_LSB = 5;
parameter int HIDELEG_HARD0_2_WIDTH = 1;
parameter logic [0:0] HIDELEG_HARD0_2_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HIDELEG_HARD0_2_MASK = 64'h0000000000000020;
parameter string HIDELEG_HARD0_2_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_1_MSB = 3;
parameter int HIDELEG_HARD0_1_LSB = 3;
parameter int HIDELEG_HARD0_1_WIDTH = 1;
parameter logic [0:0] HIDELEG_HARD0_1_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] HIDELEG_HARD0_1_MASK = 64'h0000000000000008;
parameter string HIDELEG_HARD0_1_SW_TYPE = "WARL";

parameter int HIDELEG_VSSIP_MSB = 2;
parameter int HIDELEG_VSSIP_LSB = 2;
parameter int HIDELEG_VSSIP_WIDTH = 1;
parameter logic [0:0] HIDELEG_VSSIP_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HIDELEG_VSSIP_MASK = 64'h0000000000000004;
parameter string HIDELEG_VSSIP_SW_TYPE = "WARL";

parameter int HIDELEG_HARD0_0_MSB = 1;
parameter int HIDELEG_HARD0_0_LSB = 1;
parameter int HIDELEG_HARD0_0_WIDTH = 1;
parameter logic [0:0] HIDELEG_HARD0_0_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HIDELEG_HARD0_0_MASK = 64'h0000000000000002;
parameter string HIDELEG_HARD0_0_SW_TYPE = "WARL";


// HVIP CSR Field Defines
parameter int HVIP_LCOFIP_VIRT_MSB = 13;
parameter int HVIP_LCOFIP_VIRT_LSB = 13;
parameter int HVIP_LCOFIP_VIRT_WIDTH = 1;
parameter logic [0:0] HVIP_LCOFIP_VIRT_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] HVIP_LCOFIP_VIRT_MASK = 64'h0000000000002000;
parameter string HVIP_LCOFIP_VIRT_SW_TYPE = "WARL";

parameter int HVIP_HARD0_5_MSB = 12;
parameter int HVIP_HARD0_5_LSB = 11;
parameter int HVIP_HARD0_5_WIDTH = 2;
parameter logic [1:0] HVIP_HARD0_5_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] HVIP_HARD0_5_MASK = 64'h0000000000001800;
parameter string HVIP_HARD0_5_SW_TYPE = "WARL";

parameter int HVIP_VSEIP_VIRT_MSB = 10;
parameter int HVIP_VSEIP_VIRT_LSB = 10;
parameter int HVIP_VSEIP_VIRT_WIDTH = 1;
parameter logic [0:0] HVIP_VSEIP_VIRT_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] HVIP_VSEIP_VIRT_MASK = 64'h0000000000000400;
parameter string HVIP_VSEIP_VIRT_SW_TYPE = "WARL";

parameter int HVIP_HARD0_4_MSB = 9;
parameter int HVIP_HARD0_4_LSB = 9;
parameter int HVIP_HARD0_4_WIDTH = 1;
parameter logic [0:0] HVIP_HARD0_4_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HVIP_HARD0_4_MASK = 64'h0000000000000200;
parameter string HVIP_HARD0_4_SW_TYPE = "WARL";

parameter int HVIP_HARD0_3_MSB = 7;
parameter int HVIP_HARD0_3_LSB = 7;
parameter int HVIP_HARD0_3_WIDTH = 1;
parameter logic [0:0] HVIP_HARD0_3_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HVIP_HARD0_3_MASK = 64'h0000000000000080;
parameter string HVIP_HARD0_3_SW_TYPE = "WARL";

parameter int HVIP_VSTIP_VIRT_MSB = 6;
parameter int HVIP_VSTIP_VIRT_LSB = 6;
parameter int HVIP_VSTIP_VIRT_WIDTH = 1;
parameter logic [0:0] HVIP_VSTIP_VIRT_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HVIP_VSTIP_VIRT_MASK = 64'h0000000000000040;
parameter string HVIP_VSTIP_VIRT_SW_TYPE = "WARL";

parameter int HVIP_HARD0_2_MSB = 5;
parameter int HVIP_HARD0_2_LSB = 5;
parameter int HVIP_HARD0_2_WIDTH = 1;
parameter logic [0:0] HVIP_HARD0_2_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HVIP_HARD0_2_MASK = 64'h0000000000000020;
parameter string HVIP_HARD0_2_SW_TYPE = "WARL";

parameter int HVIP_HARD0_1_MSB = 3;
parameter int HVIP_HARD0_1_LSB = 3;
parameter int HVIP_HARD0_1_WIDTH = 1;
parameter logic [0:0] HVIP_HARD0_1_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] HVIP_HARD0_1_MASK = 64'h0000000000000008;
parameter string HVIP_HARD0_1_SW_TYPE = "WARL";

parameter int HVIP_VSSIP_MSB = 2;
parameter int HVIP_VSSIP_LSB = 2;
parameter int HVIP_VSSIP_WIDTH = 1;
parameter logic [0:0] HVIP_VSSIP_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HVIP_VSSIP_MASK = 64'h0000000000000004;
parameter string HVIP_VSSIP_SW_TYPE = "WARL";

parameter int HVIP_HARD0_0_MSB = 1;
parameter int HVIP_HARD0_0_LSB = 1;
parameter int HVIP_HARD0_0_WIDTH = 1;
parameter logic [0:0] HVIP_HARD0_0_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HVIP_HARD0_0_MASK = 64'h0000000000000002;
parameter string HVIP_HARD0_0_SW_TYPE = "WARL";


// HVIPRIO1 CSR Field Defines
parameter int HVIPRIO1_HVIPRIO1_MSB = 63;
parameter int HVIPRIO1_HVIPRIO1_LSB = 0;
parameter int HVIPRIO1_HVIPRIO1_WIDTH = 64;
parameter logic [63:0] HVIPRIO1_HVIPRIO1_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HVIPRIO1_HVIPRIO1_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HVIPRIO1_HVIPRIO1_SW_TYPE = "WARL";


// HVIPRIO2 CSR Field Defines
parameter int HVIPRIO2_HVIPRIO2_MSB = 63;
parameter int HVIPRIO2_HVIPRIO2_LSB = 0;
parameter int HVIPRIO2_HVIPRIO2_WIDTH = 64;
parameter logic [63:0] HVIPRIO2_HVIPRIO2_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HVIPRIO2_HVIPRIO2_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HVIPRIO2_HVIPRIO2_SW_TYPE = "WARL";


// HIP CSR Field Defines
parameter int HIP_SGEIP_MSB = 12;
parameter int HIP_SGEIP_LSB = 12;
parameter int HIP_SGEIP_WIDTH = 1;
parameter logic [0:0] HIP_SGEIP_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] HIP_SGEIP_MASK = 64'h0000000000001000;
parameter string HIP_SGEIP_SW_TYPE = "WARL";

parameter int HIP_HARD0_5_MSB = 11;
parameter int HIP_HARD0_5_LSB = 11;
parameter int HIP_HARD0_5_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_5_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] HIP_HARD0_5_MASK = 64'h0000000000000800;
parameter string HIP_HARD0_5_SW_TYPE = "WARL";

parameter int HIP_VSEIP_MSB = 10;
parameter int HIP_VSEIP_LSB = 10;
parameter int HIP_VSEIP_WIDTH = 1;
parameter logic [0:0] HIP_VSEIP_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] HIP_VSEIP_MASK = 64'h0000000000000400;
parameter string HIP_VSEIP_SW_TYPE = "WARL";

parameter int HIP_HARD0_4_MSB = 9;
parameter int HIP_HARD0_4_LSB = 9;
parameter int HIP_HARD0_4_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_4_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HIP_HARD0_4_MASK = 64'h0000000000000200;
parameter string HIP_HARD0_4_SW_TYPE = "WARL";

parameter int HIP_HARD0_3_MSB = 7;
parameter int HIP_HARD0_3_LSB = 7;
parameter int HIP_HARD0_3_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_3_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HIP_HARD0_3_MASK = 64'h0000000000000080;
parameter string HIP_HARD0_3_SW_TYPE = "WARL";

parameter int HIP_VSTIP_MSB = 6;
parameter int HIP_VSTIP_LSB = 6;
parameter int HIP_VSTIP_WIDTH = 1;
parameter logic [0:0] HIP_VSTIP_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HIP_VSTIP_MASK = 64'h0000000000000040;
parameter string HIP_VSTIP_SW_TYPE = "WARL";

parameter int HIP_HARD0_2_MSB = 5;
parameter int HIP_HARD0_2_LSB = 5;
parameter int HIP_HARD0_2_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_2_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HIP_HARD0_2_MASK = 64'h0000000000000020;
parameter string HIP_HARD0_2_SW_TYPE = "WARL";

parameter int HIP_HARD0_1_MSB = 3;
parameter int HIP_HARD0_1_LSB = 3;
parameter int HIP_HARD0_1_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_1_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] HIP_HARD0_1_MASK = 64'h0000000000000008;
parameter string HIP_HARD0_1_SW_TYPE = "WARL";

parameter int HIP_VSSIP_MSB = 2;
parameter int HIP_VSSIP_LSB = 2;
parameter int HIP_VSSIP_WIDTH = 1;
parameter logic [0:0] HIP_VSSIP_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HIP_VSSIP_MASK = 64'h0000000000000004;
parameter string HIP_VSSIP_SW_TYPE = "WARL";

parameter int HIP_HARD0_0_MSB = 1;
parameter int HIP_HARD0_0_LSB = 1;
parameter int HIP_HARD0_0_WIDTH = 1;
parameter logic [0:0] HIP_HARD0_0_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HIP_HARD0_0_MASK = 64'h0000000000000002;
parameter string HIP_HARD0_0_SW_TYPE = "WARL";


// HIE CSR Field Defines
parameter int HIE_SGEIE_MSB = 12;
parameter int HIE_SGEIE_LSB = 12;
parameter int HIE_SGEIE_WIDTH = 1;
parameter logic [0:0] HIE_SGEIE_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] HIE_SGEIE_MASK = 64'h0000000000001000;
parameter string HIE_SGEIE_SW_TYPE = "WARL";

parameter int HIE_HARD0_5_MSB = 11;
parameter int HIE_HARD0_5_LSB = 11;
parameter int HIE_HARD0_5_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_5_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] HIE_HARD0_5_MASK = 64'h0000000000000800;
parameter string HIE_HARD0_5_SW_TYPE = "WARL";

parameter int HIE_VSEIE_MSB = 10;
parameter int HIE_VSEIE_LSB = 10;
parameter int HIE_VSEIE_WIDTH = 1;
parameter logic [0:0] HIE_VSEIE_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] HIE_VSEIE_MASK = 64'h0000000000000400;
parameter string HIE_VSEIE_SW_TYPE = "WARL";

parameter int HIE_HARD0_4_MSB = 9;
parameter int HIE_HARD0_4_LSB = 9;
parameter int HIE_HARD0_4_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_4_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HIE_HARD0_4_MASK = 64'h0000000000000200;
parameter string HIE_HARD0_4_SW_TYPE = "WARL";

parameter int HIE_HARD0_3_MSB = 7;
parameter int HIE_HARD0_3_LSB = 7;
parameter int HIE_HARD0_3_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_3_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HIE_HARD0_3_MASK = 64'h0000000000000080;
parameter string HIE_HARD0_3_SW_TYPE = "WARL";

parameter int HIE_VSTIE_MSB = 6;
parameter int HIE_VSTIE_LSB = 6;
parameter int HIE_VSTIE_WIDTH = 1;
parameter logic [0:0] HIE_VSTIE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HIE_VSTIE_MASK = 64'h0000000000000040;
parameter string HIE_VSTIE_SW_TYPE = "WARL";

parameter int HIE_HARD0_2_MSB = 5;
parameter int HIE_HARD0_2_LSB = 5;
parameter int HIE_HARD0_2_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_2_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HIE_HARD0_2_MASK = 64'h0000000000000020;
parameter string HIE_HARD0_2_SW_TYPE = "WARL";

parameter int HIE_HARD0_1_MSB = 3;
parameter int HIE_HARD0_1_LSB = 3;
parameter int HIE_HARD0_1_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_1_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] HIE_HARD0_1_MASK = 64'h0000000000000008;
parameter string HIE_HARD0_1_SW_TYPE = "WARL";

parameter int HIE_VSSIE_MSB = 2;
parameter int HIE_VSSIE_LSB = 2;
parameter int HIE_VSSIE_WIDTH = 1;
parameter logic [0:0] HIE_VSSIE_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HIE_VSSIE_MASK = 64'h0000000000000004;
parameter string HIE_VSSIE_SW_TYPE = "WARL";

parameter int HIE_HARD0_0_MSB = 1;
parameter int HIE_HARD0_0_LSB = 1;
parameter int HIE_HARD0_0_WIDTH = 1;
parameter logic [0:0] HIE_HARD0_0_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HIE_HARD0_0_MASK = 64'h0000000000000002;
parameter string HIE_HARD0_0_SW_TYPE = "WARL";


// HGEIP CSR Field Defines
parameter int HGEIP_GUESTEXTERNALINTERRUPTS_MSB = 63;
parameter int HGEIP_GUESTEXTERNALINTERRUPTS_LSB = 1;
parameter int HGEIP_GUESTEXTERNALINTERRUPTS_WIDTH = 63;
parameter logic [62:0] HGEIP_GUESTEXTERNALINTERRUPTS_RESET = 64'h0000000000000000[63:1];
parameter logic [63:0] HGEIP_GUESTEXTERNALINTERRUPTS_MASK = 64'hFFFFFFFFFFFFFFFE;
parameter string HGEIP_GUESTEXTERNALINTERRUPTS_SW_TYPE = "WARL";

parameter int HGEIP_HARD0_MSB = 0;
parameter int HGEIP_HARD0_LSB = 0;
parameter int HGEIP_HARD0_WIDTH = 1;
parameter logic [0:0] HGEIP_HARD0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] HGEIP_HARD0_MASK = 64'h0000000000000001;
parameter string HGEIP_HARD0_SW_TYPE = "WARL";


// HGEIE CSR Field Defines
parameter int HGEIE_HARD0_1_MSB = 63;
parameter int HGEIE_HARD0_1_LSB = 6;
parameter int HGEIE_HARD0_1_WIDTH = 58;
parameter logic [57:0] HGEIE_HARD0_1_RESET = 64'h0000000000000000[63:6];
parameter logic [63:0] HGEIE_HARD0_1_MASK = 64'hFFFFFFFFFFFFFFC0;
parameter string HGEIE_HARD0_1_SW_TYPE = "WARL";

parameter int HGEIE_GUESTEXTERNALINTERRUPTSWARL_MSB = 5;
parameter int HGEIE_GUESTEXTERNALINTERRUPTSWARL_LSB = 1;
parameter int HGEIE_GUESTEXTERNALINTERRUPTSWARL_WIDTH = 5;
parameter logic [4:0] HGEIE_GUESTEXTERNALINTERRUPTSWARL_RESET = 64'h0000000000000000[5:1];
parameter logic [63:0] HGEIE_GUESTEXTERNALINTERRUPTSWARL_MASK = 64'h000000000000003E;
parameter string HGEIE_GUESTEXTERNALINTERRUPTSWARL_SW_TYPE = "WARL";

parameter int HGEIE_HARD0_0_MSB = 0;
parameter int HGEIE_HARD0_0_LSB = 0;
parameter int HGEIE_HARD0_0_WIDTH = 1;
parameter logic [0:0] HGEIE_HARD0_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] HGEIE_HARD0_0_MASK = 64'h0000000000000001;
parameter string HGEIE_HARD0_0_SW_TYPE = "WARL";


// HENVCFG CSR Field Defines
parameter int HENVCFG_VSTCE_MSB = 63;
parameter int HENVCFG_VSTCE_LSB = 63;
parameter int HENVCFG_VSTCE_WIDTH = 1;
parameter logic [0:0] HENVCFG_VSTCE_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] HENVCFG_VSTCE_MASK = 64'h8000000000000000;
parameter string HENVCFG_VSTCE_SW_TYPE = "WARL";

parameter int HENVCFG_PBMTE_MSB = 62;
parameter int HENVCFG_PBMTE_LSB = 62;
parameter int HENVCFG_PBMTE_WIDTH = 1;
parameter logic [0:0] HENVCFG_PBMTE_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] HENVCFG_PBMTE_MASK = 64'h4000000000000000;
parameter string HENVCFG_PBMTE_SW_TYPE = "WARL";

parameter int HENVCFG_HADE_MSB = 61;
parameter int HENVCFG_HADE_LSB = 61;
parameter int HENVCFG_HADE_WIDTH = 1;
parameter logic [0:0] HENVCFG_HADE_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] HENVCFG_HADE_MASK = 64'h2000000000000000;
parameter string HENVCFG_HADE_SW_TYPE = "WARL";

parameter int HENVCFG_WPRI_2_MSB = 60;
parameter int HENVCFG_WPRI_2_LSB = 34;
parameter int HENVCFG_WPRI_2_WIDTH = 27;
parameter logic [26:0] HENVCFG_WPRI_2_RESET = 64'h0000000000000000[60:34];
parameter logic [63:0] HENVCFG_WPRI_2_MASK = 64'h1FFFFFFC00000000;
parameter string HENVCFG_WPRI_2_SW_TYPE = "WPRI";

parameter int HENVCFG_PMM_MSB = 33;
parameter int HENVCFG_PMM_LSB = 32;
parameter int HENVCFG_PMM_WIDTH = 2;
parameter logic [1:0] HENVCFG_PMM_RESET = 64'h0000000000000000[33:32];
parameter logic [63:0] HENVCFG_PMM_MASK = 64'h0000000300000000;
parameter string HENVCFG_PMM_SW_TYPE = "WARL";

parameter int HENVCFG_WPRI_1_MSB = 31;
parameter int HENVCFG_WPRI_1_LSB = 8;
parameter int HENVCFG_WPRI_1_WIDTH = 24;
parameter logic [23:0] HENVCFG_WPRI_1_RESET = 64'h0000000000000000[31:8];
parameter logic [63:0] HENVCFG_WPRI_1_MASK = 64'h00000000FFFFFF00;
parameter string HENVCFG_WPRI_1_SW_TYPE = "WPRI";

parameter int HENVCFG_CBZE_MSB = 7;
parameter int HENVCFG_CBZE_LSB = 7;
parameter int HENVCFG_CBZE_WIDTH = 1;
parameter logic [0:0] HENVCFG_CBZE_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HENVCFG_CBZE_MASK = 64'h0000000000000080;
parameter string HENVCFG_CBZE_SW_TYPE = "WARL";

parameter int HENVCFG_CBCFE_MSB = 6;
parameter int HENVCFG_CBCFE_LSB = 6;
parameter int HENVCFG_CBCFE_WIDTH = 1;
parameter logic [0:0] HENVCFG_CBCFE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HENVCFG_CBCFE_MASK = 64'h0000000000000040;
parameter string HENVCFG_CBCFE_SW_TYPE = "WARL";

parameter int HENVCFG_CBIE_MSB = 5;
parameter int HENVCFG_CBIE_LSB = 4;
parameter int HENVCFG_CBIE_WIDTH = 2;
parameter logic [1:0] HENVCFG_CBIE_RESET = 64'h0000000000000000[5:4];
parameter logic [63:0] HENVCFG_CBIE_MASK = 64'h0000000000000030;
parameter string HENVCFG_CBIE_SW_TYPE = "WARL";

parameter int HENVCFG_WPRI_0_MSB = 3;
parameter int HENVCFG_WPRI_0_LSB = 1;
parameter int HENVCFG_WPRI_0_WIDTH = 3;
parameter logic [2:0] HENVCFG_WPRI_0_RESET = 64'h0000000000000000[3:1];
parameter logic [63:0] HENVCFG_WPRI_0_MASK = 64'h000000000000000E;
parameter string HENVCFG_WPRI_0_SW_TYPE = "WPRI";

parameter int HENVCFG_FIOM_MSB = 0;
parameter int HENVCFG_FIOM_LSB = 0;
parameter int HENVCFG_FIOM_WIDTH = 1;
parameter logic [0:0] HENVCFG_FIOM_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] HENVCFG_FIOM_MASK = 64'h0000000000000001;
parameter string HENVCFG_FIOM_SW_TYPE = "WARL";


// HCOUNTEREN CSR Field Defines
parameter int HCOUNTEREN_HPM31_MSB = 31;
parameter int HCOUNTEREN_HPM31_LSB = 31;
parameter int HCOUNTEREN_HPM31_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM31_RESET = 64'h0000000000000000[31:31];
parameter logic [63:0] HCOUNTEREN_HPM31_MASK = 64'h0000000080000000;
parameter string HCOUNTEREN_HPM31_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM30_MSB = 30;
parameter int HCOUNTEREN_HPM30_LSB = 30;
parameter int HCOUNTEREN_HPM30_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM30_RESET = 64'h0000000000000000[30:30];
parameter logic [63:0] HCOUNTEREN_HPM30_MASK = 64'h0000000040000000;
parameter string HCOUNTEREN_HPM30_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM29_MSB = 29;
parameter int HCOUNTEREN_HPM29_LSB = 29;
parameter int HCOUNTEREN_HPM29_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM29_RESET = 64'h0000000000000000[29:29];
parameter logic [63:0] HCOUNTEREN_HPM29_MASK = 64'h0000000020000000;
parameter string HCOUNTEREN_HPM29_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM28_MSB = 28;
parameter int HCOUNTEREN_HPM28_LSB = 28;
parameter int HCOUNTEREN_HPM28_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM28_RESET = 64'h0000000000000000[28:28];
parameter logic [63:0] HCOUNTEREN_HPM28_MASK = 64'h0000000010000000;
parameter string HCOUNTEREN_HPM28_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM27_MSB = 27;
parameter int HCOUNTEREN_HPM27_LSB = 27;
parameter int HCOUNTEREN_HPM27_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM27_RESET = 64'h0000000000000000[27:27];
parameter logic [63:0] HCOUNTEREN_HPM27_MASK = 64'h0000000008000000;
parameter string HCOUNTEREN_HPM27_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM26_MSB = 26;
parameter int HCOUNTEREN_HPM26_LSB = 26;
parameter int HCOUNTEREN_HPM26_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM26_RESET = 64'h0000000000000000[26:26];
parameter logic [63:0] HCOUNTEREN_HPM26_MASK = 64'h0000000004000000;
parameter string HCOUNTEREN_HPM26_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM25_MSB = 25;
parameter int HCOUNTEREN_HPM25_LSB = 25;
parameter int HCOUNTEREN_HPM25_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM25_RESET = 64'h0000000000000000[25:25];
parameter logic [63:0] HCOUNTEREN_HPM25_MASK = 64'h0000000002000000;
parameter string HCOUNTEREN_HPM25_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM24_MSB = 24;
parameter int HCOUNTEREN_HPM24_LSB = 24;
parameter int HCOUNTEREN_HPM24_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM24_RESET = 64'h0000000000000000[24:24];
parameter logic [63:0] HCOUNTEREN_HPM24_MASK = 64'h0000000001000000;
parameter string HCOUNTEREN_HPM24_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM23_MSB = 23;
parameter int HCOUNTEREN_HPM23_LSB = 23;
parameter int HCOUNTEREN_HPM23_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM23_RESET = 64'h0000000000000000[23:23];
parameter logic [63:0] HCOUNTEREN_HPM23_MASK = 64'h0000000000800000;
parameter string HCOUNTEREN_HPM23_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM22_MSB = 22;
parameter int HCOUNTEREN_HPM22_LSB = 22;
parameter int HCOUNTEREN_HPM22_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM22_RESET = 64'h0000000000000000[22:22];
parameter logic [63:0] HCOUNTEREN_HPM22_MASK = 64'h0000000000400000;
parameter string HCOUNTEREN_HPM22_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM21_MSB = 21;
parameter int HCOUNTEREN_HPM21_LSB = 21;
parameter int HCOUNTEREN_HPM21_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM21_RESET = 64'h0000000000000000[21:21];
parameter logic [63:0] HCOUNTEREN_HPM21_MASK = 64'h0000000000200000;
parameter string HCOUNTEREN_HPM21_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM20_MSB = 20;
parameter int HCOUNTEREN_HPM20_LSB = 20;
parameter int HCOUNTEREN_HPM20_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM20_RESET = 64'h0000000000000000[20:20];
parameter logic [63:0] HCOUNTEREN_HPM20_MASK = 64'h0000000000100000;
parameter string HCOUNTEREN_HPM20_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM19_MSB = 19;
parameter int HCOUNTEREN_HPM19_LSB = 19;
parameter int HCOUNTEREN_HPM19_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM19_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] HCOUNTEREN_HPM19_MASK = 64'h0000000000080000;
parameter string HCOUNTEREN_HPM19_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM18_MSB = 18;
parameter int HCOUNTEREN_HPM18_LSB = 18;
parameter int HCOUNTEREN_HPM18_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM18_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] HCOUNTEREN_HPM18_MASK = 64'h0000000000040000;
parameter string HCOUNTEREN_HPM18_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM17_MSB = 17;
parameter int HCOUNTEREN_HPM17_LSB = 17;
parameter int HCOUNTEREN_HPM17_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM17_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] HCOUNTEREN_HPM17_MASK = 64'h0000000000020000;
parameter string HCOUNTEREN_HPM17_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM16_MSB = 16;
parameter int HCOUNTEREN_HPM16_LSB = 16;
parameter int HCOUNTEREN_HPM16_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM16_RESET = 64'h0000000000000000[16:16];
parameter logic [63:0] HCOUNTEREN_HPM16_MASK = 64'h0000000000010000;
parameter string HCOUNTEREN_HPM16_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM15_MSB = 15;
parameter int HCOUNTEREN_HPM15_LSB = 15;
parameter int HCOUNTEREN_HPM15_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM15_RESET = 64'h0000000000000000[15:15];
parameter logic [63:0] HCOUNTEREN_HPM15_MASK = 64'h0000000000008000;
parameter string HCOUNTEREN_HPM15_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM14_MSB = 14;
parameter int HCOUNTEREN_HPM14_LSB = 14;
parameter int HCOUNTEREN_HPM14_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM14_RESET = 64'h0000000000000000[14:14];
parameter logic [63:0] HCOUNTEREN_HPM14_MASK = 64'h0000000000004000;
parameter string HCOUNTEREN_HPM14_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM13_MSB = 13;
parameter int HCOUNTEREN_HPM13_LSB = 13;
parameter int HCOUNTEREN_HPM13_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM13_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] HCOUNTEREN_HPM13_MASK = 64'h0000000000002000;
parameter string HCOUNTEREN_HPM13_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM12_MSB = 12;
parameter int HCOUNTEREN_HPM12_LSB = 12;
parameter int HCOUNTEREN_HPM12_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM12_RESET = 64'h0000000000000000[12:12];
parameter logic [63:0] HCOUNTEREN_HPM12_MASK = 64'h0000000000001000;
parameter string HCOUNTEREN_HPM12_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM11_MSB = 11;
parameter int HCOUNTEREN_HPM11_LSB = 11;
parameter int HCOUNTEREN_HPM11_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM11_RESET = 64'h0000000000000000[11:11];
parameter logic [63:0] HCOUNTEREN_HPM11_MASK = 64'h0000000000000800;
parameter string HCOUNTEREN_HPM11_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM10_MSB = 10;
parameter int HCOUNTEREN_HPM10_LSB = 10;
parameter int HCOUNTEREN_HPM10_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM10_RESET = 64'h0000000000000000[10:10];
parameter logic [63:0] HCOUNTEREN_HPM10_MASK = 64'h0000000000000400;
parameter string HCOUNTEREN_HPM10_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM9_MSB = 9;
parameter int HCOUNTEREN_HPM9_LSB = 9;
parameter int HCOUNTEREN_HPM9_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM9_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HCOUNTEREN_HPM9_MASK = 64'h0000000000000200;
parameter string HCOUNTEREN_HPM9_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM8_MSB = 8;
parameter int HCOUNTEREN_HPM8_LSB = 8;
parameter int HCOUNTEREN_HPM8_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM8_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] HCOUNTEREN_HPM8_MASK = 64'h0000000000000100;
parameter string HCOUNTEREN_HPM8_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM7_MSB = 7;
parameter int HCOUNTEREN_HPM7_LSB = 7;
parameter int HCOUNTEREN_HPM7_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM7_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] HCOUNTEREN_HPM7_MASK = 64'h0000000000000080;
parameter string HCOUNTEREN_HPM7_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM6_MSB = 6;
parameter int HCOUNTEREN_HPM6_LSB = 6;
parameter int HCOUNTEREN_HPM6_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM6_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] HCOUNTEREN_HPM6_MASK = 64'h0000000000000040;
parameter string HCOUNTEREN_HPM6_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM5_MSB = 5;
parameter int HCOUNTEREN_HPM5_LSB = 5;
parameter int HCOUNTEREN_HPM5_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM5_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] HCOUNTEREN_HPM5_MASK = 64'h0000000000000020;
parameter string HCOUNTEREN_HPM5_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM4_MSB = 4;
parameter int HCOUNTEREN_HPM4_LSB = 4;
parameter int HCOUNTEREN_HPM4_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM4_RESET = 64'h0000000000000000[4:4];
parameter logic [63:0] HCOUNTEREN_HPM4_MASK = 64'h0000000000000010;
parameter string HCOUNTEREN_HPM4_SW_TYPE = "WARL";

parameter int HCOUNTEREN_HPM3_MSB = 3;
parameter int HCOUNTEREN_HPM3_LSB = 3;
parameter int HCOUNTEREN_HPM3_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_HPM3_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] HCOUNTEREN_HPM3_MASK = 64'h0000000000000008;
parameter string HCOUNTEREN_HPM3_SW_TYPE = "WARL";

parameter int HCOUNTEREN_IR_MSB = 2;
parameter int HCOUNTEREN_IR_LSB = 2;
parameter int HCOUNTEREN_IR_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_IR_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HCOUNTEREN_IR_MASK = 64'h0000000000000004;
parameter string HCOUNTEREN_IR_SW_TYPE = "WARL";

parameter int HCOUNTEREN_TM_MSB = 1;
parameter int HCOUNTEREN_TM_LSB = 1;
parameter int HCOUNTEREN_TM_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_TM_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HCOUNTEREN_TM_MASK = 64'h0000000000000002;
parameter string HCOUNTEREN_TM_SW_TYPE = "WARL";

parameter int HCOUNTEREN_CY_MSB = 0;
parameter int HCOUNTEREN_CY_LSB = 0;
parameter int HCOUNTEREN_CY_WIDTH = 1;
parameter logic [0:0] HCOUNTEREN_CY_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] HCOUNTEREN_CY_MASK = 64'h0000000000000001;
parameter string HCOUNTEREN_CY_SW_TYPE = "WARL";


// HTIMEDELTA CSR Field Defines
parameter int HTIMEDELTA_HTIMEDELTA_MSB = 63;
parameter int HTIMEDELTA_HTIMEDELTA_LSB = 0;
parameter int HTIMEDELTA_HTIMEDELTA_WIDTH = 64;
parameter logic [63:0] HTIMEDELTA_HTIMEDELTA_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HTIMEDELTA_HTIMEDELTA_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HTIMEDELTA_HTIMEDELTA_SW_TYPE = "WARL";


// HTVAL CSR Field Defines
parameter int HTVAL_HTVAL_MSB = 63;
parameter int HTVAL_HTVAL_LSB = 0;
parameter int HTVAL_HTVAL_WIDTH = 64;
parameter logic [63:0] HTVAL_HTVAL_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HTVAL_HTVAL_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HTVAL_HTVAL_SW_TYPE = "WARL";


// HTINST CSR Field Defines
parameter int HTINST_HTINST_MSB = 63;
parameter int HTINST_HTINST_LSB = 0;
parameter int HTINST_HTINST_WIDTH = 64;
parameter logic [63:0] HTINST_HTINST_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] HTINST_HTINST_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string HTINST_HTINST_SW_TYPE = "WARL";


// HGATP CSR Field Defines
parameter int HGATP_MODE_MSB = 63;
parameter int HGATP_MODE_LSB = 60;
parameter int HGATP_MODE_WIDTH = 4;
parameter logic [3:0] HGATP_MODE_RESET = 64'h0000000000000000[63:60];
parameter logic [63:0] HGATP_MODE_MASK = 64'hF000000000000000;
parameter string HGATP_MODE_SW_TYPE = "WARL";

parameter int HGATP_WARL0_MSB = 59;
parameter int HGATP_WARL0_LSB = 58;
parameter int HGATP_WARL0_WIDTH = 2;
parameter logic [1:0] HGATP_WARL0_RESET = 64'h0000000000000000[59:58];
parameter logic [63:0] HGATP_WARL0_MASK = 64'h0C00000000000000;
parameter string HGATP_WARL0_SW_TYPE = "WARL";

parameter int HGATP_VMID_MSB = 57;
parameter int HGATP_VMID_LSB = 44;
parameter int HGATP_VMID_WIDTH = 14;
parameter logic [13:0] HGATP_VMID_RESET = 64'h0000000000000000[57:44];
parameter logic [63:0] HGATP_VMID_MASK = 64'h03FFF00000000000;
parameter string HGATP_VMID_SW_TYPE = "WARL";

parameter int HGATP_PPN_MSB = 43;
parameter int HGATP_PPN_LSB = 0;
parameter int HGATP_PPN_WIDTH = 44;
parameter logic [43:0] HGATP_PPN_RESET = 64'h0000000000000000[43:0];
parameter logic [63:0] HGATP_PPN_MASK = 64'h00000FFFFFFFFFFF;
parameter string HGATP_PPN_SW_TYPE = "WARL";


// HVIEN CSR Field Defines
parameter int HVIEN_LCOFIP_MSB = 13;
parameter int HVIEN_LCOFIP_LSB = 13;
parameter int HVIEN_LCOFIP_WIDTH = 1;
parameter logic [0:0] HVIEN_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] HVIEN_LCOFIP_MASK = 64'h0000000000002000;
parameter string HVIEN_LCOFIP_SW_TYPE = "WARL";

parameter int HVIEN_HARD0_0_MSB = 12;
parameter int HVIEN_HARD0_0_LSB = 0;
parameter int HVIEN_HARD0_0_WIDTH = 13;
parameter logic [12:0] HVIEN_HARD0_0_RESET = 64'h0000000000000000[12:0];
parameter logic [63:0] HVIEN_HARD0_0_MASK = 64'h0000000000001FFF;
parameter string HVIEN_HARD0_0_SW_TYPE = "WARL";


// HVICTL CSR Field Defines
parameter int HVICTL_VTI_MSB = 30;
parameter int HVICTL_VTI_LSB = 30;
parameter int HVICTL_VTI_WIDTH = 1;
parameter logic [0:0] HVICTL_VTI_RESET = 64'h0000000000000000[30:30];
parameter logic [63:0] HVICTL_VTI_MASK = 64'h0000000040000000;
parameter string HVICTL_VTI_SW_TYPE = "WARL";

parameter int HVICTL_IID_MSB = 21;
parameter int HVICTL_IID_LSB = 16;
parameter int HVICTL_IID_WIDTH = 6;
parameter logic [5:0] HVICTL_IID_RESET = 64'h0000000000000000[21:16];
parameter logic [63:0] HVICTL_IID_MASK = 64'h00000000003F0000;
parameter string HVICTL_IID_SW_TYPE = "WARL";

parameter int HVICTL_DPR_MSB = 9;
parameter int HVICTL_DPR_LSB = 9;
parameter int HVICTL_DPR_WIDTH = 1;
parameter logic [0:0] HVICTL_DPR_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] HVICTL_DPR_MASK = 64'h0000000000000200;
parameter string HVICTL_DPR_SW_TYPE = "WARL";

parameter int HVICTL_IPRIOM_MSB = 8;
parameter int HVICTL_IPRIOM_LSB = 8;
parameter int HVICTL_IPRIOM_WIDTH = 1;
parameter logic [0:0] HVICTL_IPRIOM_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] HVICTL_IPRIOM_MASK = 64'h0000000000000100;
parameter string HVICTL_IPRIOM_SW_TYPE = "WARL";

parameter int HVICTL_IPRIO_MSB = 7;
parameter int HVICTL_IPRIO_LSB = 0;
parameter int HVICTL_IPRIO_WIDTH = 8;
parameter logic [7:0] HVICTL_IPRIO_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] HVICTL_IPRIO_MASK = 64'h00000000000000FF;
parameter string HVICTL_IPRIO_SW_TYPE = "WARL";


// VSSTATUS CSR Field Defines
parameter int VSSTATUS_SD_MSB = 63;
parameter int VSSTATUS_SD_LSB = 63;
parameter int VSSTATUS_SD_WIDTH = 1;
parameter logic [0:0] VSSTATUS_SD_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] VSSTATUS_SD_MASK = 64'h8000000000000000;
parameter string VSSTATUS_SD_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_6_MSB = 62;
parameter int VSSTATUS_VSSTATUS_WPRI_6_LSB = 34;
parameter int VSSTATUS_VSSTATUS_WPRI_6_WIDTH = 29;
parameter logic [28:0] VSSTATUS_VSSTATUS_WPRI_6_RESET = 64'h0000000000000000[62:34];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_6_MASK = 64'h7FFFFFFC00000000;
parameter string VSSTATUS_VSSTATUS_WPRI_6_SW_TYPE = "WPRI";

parameter int VSSTATUS_UXL_MSB = 33;
parameter int VSSTATUS_UXL_LSB = 32;
parameter int VSSTATUS_UXL_WIDTH = 2;
parameter logic [1:0] VSSTATUS_UXL_RESET = 64'h0000000000000002[33:32];
parameter logic [63:0] VSSTATUS_UXL_MASK = 64'h0000000300000000;
parameter string VSSTATUS_UXL_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_5_MSB = 31;
parameter int VSSTATUS_VSSTATUS_WPRI_5_LSB = 20;
parameter int VSSTATUS_VSSTATUS_WPRI_5_WIDTH = 12;
parameter logic [11:0] VSSTATUS_VSSTATUS_WPRI_5_RESET = 64'h0000000000000000[31:20];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_5_MASK = 64'h00000000FFF00000;
parameter string VSSTATUS_VSSTATUS_WPRI_5_SW_TYPE = "WPRI";

parameter int VSSTATUS_MXR_MSB = 19;
parameter int VSSTATUS_MXR_LSB = 19;
parameter int VSSTATUS_MXR_WIDTH = 1;
parameter logic [0:0] VSSTATUS_MXR_RESET = 64'h0000000000000000[19:19];
parameter logic [63:0] VSSTATUS_MXR_MASK = 64'h0000000000080000;
parameter string VSSTATUS_MXR_SW_TYPE = "WARL";

parameter int VSSTATUS_SUM_MSB = 18;
parameter int VSSTATUS_SUM_LSB = 18;
parameter int VSSTATUS_SUM_WIDTH = 1;
parameter logic [0:0] VSSTATUS_SUM_RESET = 64'h0000000000000000[18:18];
parameter logic [63:0] VSSTATUS_SUM_MASK = 64'h0000000000040000;
parameter string VSSTATUS_SUM_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_4_MSB = 17;
parameter int VSSTATUS_VSSTATUS_WPRI_4_LSB = 17;
parameter int VSSTATUS_VSSTATUS_WPRI_4_WIDTH = 1;
parameter logic [0:0] VSSTATUS_VSSTATUS_WPRI_4_RESET = 64'h0000000000000000[17:17];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_4_MASK = 64'h0000000000020000;
parameter string VSSTATUS_VSSTATUS_WPRI_4_SW_TYPE = "WPRI";

parameter int VSSTATUS_XS_MSB = 16;
parameter int VSSTATUS_XS_LSB = 15;
parameter int VSSTATUS_XS_WIDTH = 2;
parameter logic [1:0] VSSTATUS_XS_RESET = 64'h0000000000000000[16:15];
parameter logic [63:0] VSSTATUS_XS_MASK = 64'h0000000000018000;
parameter string VSSTATUS_XS_SW_TYPE = "WARL";

parameter int VSSTATUS_FS_MSB = 14;
parameter int VSSTATUS_FS_LSB = 13;
parameter int VSSTATUS_FS_WIDTH = 2;
parameter logic [1:0] VSSTATUS_FS_RESET = 64'h0000000000000000[14:13];
parameter logic [63:0] VSSTATUS_FS_MASK = 64'h0000000000006000;
parameter string VSSTATUS_FS_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_3_MSB = 12;
parameter int VSSTATUS_VSSTATUS_WPRI_3_LSB = 11;
parameter int VSSTATUS_VSSTATUS_WPRI_3_WIDTH = 2;
parameter logic [1:0] VSSTATUS_VSSTATUS_WPRI_3_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_3_MASK = 64'h0000000000001800;
parameter string VSSTATUS_VSSTATUS_WPRI_3_SW_TYPE = "WPRI";

parameter int VSSTATUS_VS_MSB = 10;
parameter int VSSTATUS_VS_LSB = 9;
parameter int VSSTATUS_VS_WIDTH = 2;
parameter logic [1:0] VSSTATUS_VS_RESET = 64'h0000000000000000[10:9];
parameter logic [63:0] VSSTATUS_VS_MASK = 64'h0000000000000600;
parameter string VSSTATUS_VS_SW_TYPE = "WARL";

parameter int VSSTATUS_SPP_MSB = 8;
parameter int VSSTATUS_SPP_LSB = 8;
parameter int VSSTATUS_SPP_WIDTH = 1;
parameter logic [0:0] VSSTATUS_SPP_RESET = 64'h0000000000000000[8:8];
parameter logic [63:0] VSSTATUS_SPP_MASK = 64'h0000000000000100;
parameter string VSSTATUS_SPP_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_2_MSB = 7;
parameter int VSSTATUS_VSSTATUS_WPRI_2_LSB = 7;
parameter int VSSTATUS_VSSTATUS_WPRI_2_WIDTH = 1;
parameter logic [0:0] VSSTATUS_VSSTATUS_WPRI_2_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_2_MASK = 64'h0000000000000080;
parameter string VSSTATUS_VSSTATUS_WPRI_2_SW_TYPE = "WPRI";

parameter int VSSTATUS_UBE_MSB = 6;
parameter int VSSTATUS_UBE_LSB = 6;
parameter int VSSTATUS_UBE_WIDTH = 1;
parameter logic [0:0] VSSTATUS_UBE_RESET = 64'h0000000000000000[6:6];
parameter logic [63:0] VSSTATUS_UBE_MASK = 64'h0000000000000040;
parameter string VSSTATUS_UBE_SW_TYPE = "WARL";

parameter int VSSTATUS_SPIE_MSB = 5;
parameter int VSSTATUS_SPIE_LSB = 5;
parameter int VSSTATUS_SPIE_WIDTH = 1;
parameter logic [0:0] VSSTATUS_SPIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] VSSTATUS_SPIE_MASK = 64'h0000000000000020;
parameter string VSSTATUS_SPIE_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_1_MSB = 4;
parameter int VSSTATUS_VSSTATUS_WPRI_1_LSB = 2;
parameter int VSSTATUS_VSSTATUS_WPRI_1_WIDTH = 3;
parameter logic [2:0] VSSTATUS_VSSTATUS_WPRI_1_RESET = 64'h0000000000000000[4:2];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_1_MASK = 64'h000000000000001C;
parameter string VSSTATUS_VSSTATUS_WPRI_1_SW_TYPE = "WPRI";

parameter int VSSTATUS_SIE_MSB = 1;
parameter int VSSTATUS_SIE_LSB = 1;
parameter int VSSTATUS_SIE_WIDTH = 1;
parameter logic [0:0] VSSTATUS_SIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] VSSTATUS_SIE_MASK = 64'h0000000000000002;
parameter string VSSTATUS_SIE_SW_TYPE = "WARL";

parameter int VSSTATUS_VSSTATUS_WPRI_0_MSB = 0;
parameter int VSSTATUS_VSSTATUS_WPRI_0_LSB = 0;
parameter int VSSTATUS_VSSTATUS_WPRI_0_WIDTH = 1;
parameter logic [0:0] VSSTATUS_VSSTATUS_WPRI_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] VSSTATUS_VSSTATUS_WPRI_0_MASK = 64'h0000000000000001;
parameter string VSSTATUS_VSSTATUS_WPRI_0_SW_TYPE = "WPRI";


// VSIP CSR Field Defines
parameter int VSIP_LCOFIP_MSB = 13;
parameter int VSIP_LCOFIP_LSB = 13;
parameter int VSIP_LCOFIP_WIDTH = 1;
parameter logic [0:0] VSIP_LCOFIP_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] VSIP_LCOFIP_MASK = 64'h0000000000002000;
parameter string VSIP_LCOFIP_SW_TYPE = "WARL";

parameter int VSIP_HARD0_3_MSB = 12;
parameter int VSIP_HARD0_3_LSB = 10;
parameter int VSIP_HARD0_3_WIDTH = 3;
parameter logic [2:0] VSIP_HARD0_3_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] VSIP_HARD0_3_MASK = 64'h0000000000001C00;
parameter string VSIP_HARD0_3_SW_TYPE = "WARL";

parameter int VSIP_VSEIP_MSB = 9;
parameter int VSIP_VSEIP_LSB = 9;
parameter int VSIP_VSEIP_WIDTH = 1;
parameter logic [0:0] VSIP_VSEIP_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] VSIP_VSEIP_MASK = 64'h0000000000000200;
parameter string VSIP_VSEIP_SW_TYPE = "WARL";

parameter int VSIP_HARD0_2_MSB = 7;
parameter int VSIP_HARD0_2_LSB = 6;
parameter int VSIP_HARD0_2_WIDTH = 2;
parameter logic [1:0] VSIP_HARD0_2_RESET = 64'h0000000000000000[7:6];
parameter logic [63:0] VSIP_HARD0_2_MASK = 64'h00000000000000C0;
parameter string VSIP_HARD0_2_SW_TYPE = "WARL";

parameter int VSIP_VSTIP_MSB = 5;
parameter int VSIP_VSTIP_LSB = 5;
parameter int VSIP_VSTIP_WIDTH = 1;
parameter logic [0:0] VSIP_VSTIP_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] VSIP_VSTIP_MASK = 64'h0000000000000020;
parameter string VSIP_VSTIP_SW_TYPE = "WARL";

parameter int VSIP_HARD0_1_MSB = 3;
parameter int VSIP_HARD0_1_LSB = 2;
parameter int VSIP_HARD0_1_WIDTH = 2;
parameter logic [1:0] VSIP_HARD0_1_RESET = 64'h0000000000000000[3:2];
parameter logic [63:0] VSIP_HARD0_1_MASK = 64'h000000000000000C;
parameter string VSIP_HARD0_1_SW_TYPE = "WARL";

parameter int VSIP_VSSIP_MSB = 1;
parameter int VSIP_VSSIP_LSB = 1;
parameter int VSIP_VSSIP_WIDTH = 1;
parameter logic [0:0] VSIP_VSSIP_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] VSIP_VSSIP_MASK = 64'h0000000000000002;
parameter string VSIP_VSSIP_SW_TYPE = "WARL";


// VSIE CSR Field Defines
parameter int VSIE_LCOFIE_MSB = 13;
parameter int VSIE_LCOFIE_LSB = 13;
parameter int VSIE_LCOFIE_WIDTH = 1;
parameter logic [0:0] VSIE_LCOFIE_RESET = 64'h0000000000000000[13:13];
parameter logic [63:0] VSIE_LCOFIE_MASK = 64'h0000000000002000;
parameter string VSIE_LCOFIE_SW_TYPE = "WARL";

parameter int VSIE_HARD0_2_MSB = 12;
parameter int VSIE_HARD0_2_LSB = 10;
parameter int VSIE_HARD0_2_WIDTH = 3;
parameter logic [2:0] VSIE_HARD0_2_RESET = 64'h0000000000000000[12:10];
parameter logic [63:0] VSIE_HARD0_2_MASK = 64'h0000000000001C00;
parameter string VSIE_HARD0_2_SW_TYPE = "WARL";

parameter int VSIE_VSEIE_MSB = 9;
parameter int VSIE_VSEIE_LSB = 9;
parameter int VSIE_VSEIE_WIDTH = 1;
parameter logic [0:0] VSIE_VSEIE_RESET = 64'h0000000000000000[9:9];
parameter logic [63:0] VSIE_VSEIE_MASK = 64'h0000000000000200;
parameter string VSIE_VSEIE_SW_TYPE = "WARL";

parameter int VSIE_HARD0_1_MSB = 7;
parameter int VSIE_HARD0_1_LSB = 6;
parameter int VSIE_HARD0_1_WIDTH = 2;
parameter logic [1:0] VSIE_HARD0_1_RESET = 64'h0000000000000000[7:6];
parameter logic [63:0] VSIE_HARD0_1_MASK = 64'h00000000000000C0;
parameter string VSIE_HARD0_1_SW_TYPE = "WARL";

parameter int VSIE_VSTIE_MSB = 5;
parameter int VSIE_VSTIE_LSB = 5;
parameter int VSIE_VSTIE_WIDTH = 1;
parameter logic [0:0] VSIE_VSTIE_RESET = 64'h0000000000000000[5:5];
parameter logic [63:0] VSIE_VSTIE_MASK = 64'h0000000000000020;
parameter string VSIE_VSTIE_SW_TYPE = "WARL";

parameter int VSIE_HARD0_0_MSB = 3;
parameter int VSIE_HARD0_0_LSB = 2;
parameter int VSIE_HARD0_0_WIDTH = 2;
parameter logic [1:0] VSIE_HARD0_0_RESET = 64'h0000000000000000[3:2];
parameter logic [63:0] VSIE_HARD0_0_MASK = 64'h000000000000000C;
parameter string VSIE_HARD0_0_SW_TYPE = "WARL";

parameter int VSIE_VSSIE_MSB = 1;
parameter int VSIE_VSSIE_LSB = 1;
parameter int VSIE_VSSIE_WIDTH = 1;
parameter logic [0:0] VSIE_VSSIE_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] VSIE_VSSIE_MASK = 64'h0000000000000002;
parameter string VSIE_VSSIE_SW_TYPE = "WARL";


// VSTVEC CSR Field Defines
parameter int VSTVEC_BASESXLEN12WARL_MSB = 63;
parameter int VSTVEC_BASESXLEN12WARL_LSB = 2;
parameter int VSTVEC_BASESXLEN12WARL_WIDTH = 62;
parameter logic [61:0] VSTVEC_BASESXLEN12WARL_RESET = 64'h0000000000000000[63:2];
parameter logic [63:0] VSTVEC_BASESXLEN12WARL_MASK = 64'hFFFFFFFFFFFFFFFC;
parameter string VSTVEC_BASESXLEN12WARL_SW_TYPE = "WARL";

parameter int VSTVEC_MODE_1_MSB = 1;
parameter int VSTVEC_MODE_1_LSB = 1;
parameter int VSTVEC_MODE_1_WIDTH = 1;
parameter logic [0:0] VSTVEC_MODE_1_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] VSTVEC_MODE_1_MASK = 64'h0000000000000002;
parameter string VSTVEC_MODE_1_SW_TYPE = "WARL";

parameter int VSTVEC_MODE_0_MSB = 0;
parameter int VSTVEC_MODE_0_LSB = 0;
parameter int VSTVEC_MODE_0_WIDTH = 1;
parameter logic [0:0] VSTVEC_MODE_0_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] VSTVEC_MODE_0_MASK = 64'h0000000000000001;
parameter string VSTVEC_MODE_0_SW_TYPE = "WARL";


// VSSCRATCH CSR Field Defines
parameter int VSSCRATCH_SSCRATCH_MSB = 63;
parameter int VSSCRATCH_SSCRATCH_LSB = 0;
parameter int VSSCRATCH_SSCRATCH_WIDTH = 64;
parameter logic [63:0] VSSCRATCH_SSCRATCH_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] VSSCRATCH_SSCRATCH_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VSSCRATCH_SSCRATCH_SW_TYPE = "WARL";


// VSEPC CSR Field Defines
parameter int VSEPC_ADDR_MSB = 63;
parameter int VSEPC_ADDR_LSB = 1;
parameter int VSEPC_ADDR_WIDTH = 63;
parameter logic [62:0] VSEPC_ADDR_RESET = 64'h0000000000000000[63:1];
parameter logic [63:0] VSEPC_ADDR_MASK = 64'hFFFFFFFFFFFFFFFE;
parameter string VSEPC_ADDR_SW_TYPE = "WARL";


// VSCAUSE CSR Field Defines
parameter int VSCAUSE_INTERRUPT_MSB = 63;
parameter int VSCAUSE_INTERRUPT_LSB = 63;
parameter int VSCAUSE_INTERRUPT_WIDTH = 1;
parameter logic [0:0] VSCAUSE_INTERRUPT_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] VSCAUSE_INTERRUPT_MASK = 64'h8000000000000000;
parameter string VSCAUSE_INTERRUPT_SW_TYPE = "WARL";

parameter int VSCAUSE_EXCEPTIONCODEWLRL_MSB = 62;
parameter int VSCAUSE_EXCEPTIONCODEWLRL_LSB = 0;
parameter int VSCAUSE_EXCEPTIONCODEWLRL_WIDTH = 63;
parameter logic [62:0] VSCAUSE_EXCEPTIONCODEWLRL_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] VSCAUSE_EXCEPTIONCODEWLRL_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string VSCAUSE_EXCEPTIONCODEWLRL_SW_TYPE = "WARL";


// VSTVAL CSR Field Defines
parameter int VSTVAL_VSTVAL_MSB = 63;
parameter int VSTVAL_VSTVAL_LSB = 0;
parameter int VSTVAL_VSTVAL_WIDTH = 64;
parameter logic [63:0] VSTVAL_VSTVAL_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] VSTVAL_VSTVAL_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VSTVAL_VSTVAL_SW_TYPE = "WARL";


// VSTIMECMP CSR Field Defines
parameter int VSTIMECMP_VSTIMECMP_MSB = 63;
parameter int VSTIMECMP_VSTIMECMP_LSB = 0;
parameter int VSTIMECMP_VSTIMECMP_WIDTH = 64;
parameter logic [63:0] VSTIMECMP_VSTIMECMP_RESET = 64'h00000000FFFFFFFF[63:0];
parameter logic [63:0] VSTIMECMP_VSTIMECMP_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VSTIMECMP_VSTIMECMP_SW_TYPE = "WARL";


// VSATP CSR Field Defines
parameter int VSATP_MODE_MSB = 63;
parameter int VSATP_MODE_LSB = 60;
parameter int VSATP_MODE_WIDTH = 4;
parameter logic [3:0] VSATP_MODE_RESET = 64'h0000000000000000[63:60];
parameter logic [63:0] VSATP_MODE_MASK = 64'hF000000000000000;
parameter string VSATP_MODE_SW_TYPE = "WARL";

parameter int VSATP_ASID_MSB = 59;
parameter int VSATP_ASID_LSB = 44;
parameter int VSATP_ASID_WIDTH = 16;
parameter logic [15:0] VSATP_ASID_RESET = 64'h0000000000000000[59:44];
parameter logic [63:0] VSATP_ASID_MASK = 64'h0FFFF00000000000;
parameter string VSATP_ASID_SW_TYPE = "WARL";

parameter int VSATP_PPN_MSB = 43;
parameter int VSATP_PPN_LSB = 0;
parameter int VSATP_PPN_WIDTH = 44;
parameter logic [43:0] VSATP_PPN_RESET = 64'h0000000000000000[43:0];
parameter logic [63:0] VSATP_PPN_MASK = 64'h00000FFFFFFFFFFF;
parameter string VSATP_PPN_SW_TYPE = "WARL";


// VSISELECT CSR Field Defines
parameter int VSISELECT_RSVD_63_9_MSB = 63;
parameter int VSISELECT_RSVD_63_9_LSB = 9;
parameter int VSISELECT_RSVD_63_9_WIDTH = 55;
parameter logic [54:0] VSISELECT_RSVD_63_9_RESET = 64'h0000000000000000[63:9];
parameter logic [63:0] VSISELECT_RSVD_63_9_MASK = 64'hFFFFFFFFFFFFFE00;
parameter string VSISELECT_RSVD_63_9_SW_TYPE = "WARL";

parameter int VSISELECT_INTERRUPTS_MSB = 8;
parameter int VSISELECT_INTERRUPTS_LSB = 0;
parameter int VSISELECT_INTERRUPTS_WIDTH = 9;
parameter logic [8:0] VSISELECT_INTERRUPTS_RESET = 64'h0000000000000000[8:0];
parameter logic [63:0] VSISELECT_INTERRUPTS_MASK = 64'h00000000000001FF;
parameter string VSISELECT_INTERRUPTS_SW_TYPE = "WARL";


// VSIREG CSR Field Defines
parameter int VSIREG_SIREG_MSB = 63;
parameter int VSIREG_SIREG_LSB = 0;
parameter int VSIREG_SIREG_WIDTH = 64;
parameter logic [63:0] VSIREG_SIREG_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] VSIREG_SIREG_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string VSIREG_SIREG_SW_TYPE = "WARL";


// VSTOPEI CSR Field Defines
parameter int VSTOPEI_RSVD_63_27_MSB = 63;
parameter int VSTOPEI_RSVD_63_27_LSB = 27;
parameter int VSTOPEI_RSVD_63_27_WIDTH = 37;
parameter logic [36:0] VSTOPEI_RSVD_63_27_RESET = 64'h0000000000000000[63:27];
parameter logic [63:0] VSTOPEI_RSVD_63_27_MASK = 64'hFFFFFFFFF8000000;
parameter string VSTOPEI_RSVD_63_27_SW_TYPE = "WARL";

parameter int VSTOPEI_IDENTITY_MSB = 26;
parameter int VSTOPEI_IDENTITY_LSB = 16;
parameter int VSTOPEI_IDENTITY_WIDTH = 11;
parameter logic [10:0] VSTOPEI_IDENTITY_RESET = 64'h0000000000000000[26:16];
parameter logic [63:0] VSTOPEI_IDENTITY_MASK = 64'h0000000007FF0000;
parameter string VSTOPEI_IDENTITY_SW_TYPE = "WARL";

parameter int VSTOPEI_RSVD_15_11_MSB = 15;
parameter int VSTOPEI_RSVD_15_11_LSB = 11;
parameter int VSTOPEI_RSVD_15_11_WIDTH = 5;
parameter logic [4:0] VSTOPEI_RSVD_15_11_RESET = 64'h0000000000000000[15:11];
parameter logic [63:0] VSTOPEI_RSVD_15_11_MASK = 64'h000000000000F800;
parameter string VSTOPEI_RSVD_15_11_SW_TYPE = "WARL";

parameter int VSTOPEI_PRIORITY_MSB = 10;
parameter int VSTOPEI_PRIORITY_LSB = 0;
parameter int VSTOPEI_PRIORITY_WIDTH = 11;
parameter logic [10:0] VSTOPEI_PRIORITY_RESET = 64'h0000000000000000[10:0];
parameter logic [63:0] VSTOPEI_PRIORITY_MASK = 64'h00000000000007FF;
parameter string VSTOPEI_PRIORITY_SW_TYPE = "WARL";


// VSTOPI CSR Field Defines
parameter int VSTOPI_RSVD_63_28_MSB = 63;
parameter int VSTOPI_RSVD_63_28_LSB = 28;
parameter int VSTOPI_RSVD_63_28_WIDTH = 36;
parameter logic [35:0] VSTOPI_RSVD_63_28_RESET = 64'h0000000000000000[63:28];
parameter logic [63:0] VSTOPI_RSVD_63_28_MASK = 64'hFFFFFFFFF0000000;
parameter string VSTOPI_RSVD_63_28_SW_TYPE = "WARL";

parameter int VSTOPI_IID_MSB = 27;
parameter int VSTOPI_IID_LSB = 16;
parameter int VSTOPI_IID_WIDTH = 12;
parameter logic [11:0] VSTOPI_IID_RESET = 64'h0000000000000000[27:16];
parameter logic [63:0] VSTOPI_IID_MASK = 64'h000000000FFF0000;
parameter string VSTOPI_IID_SW_TYPE = "WARL";

parameter int VSTOPI_RSVD_15_8_MSB = 15;
parameter int VSTOPI_RSVD_15_8_LSB = 8;
parameter int VSTOPI_RSVD_15_8_WIDTH = 8;
parameter logic [7:0] VSTOPI_RSVD_15_8_RESET = 64'h0000000000000000[15:8];
parameter logic [63:0] VSTOPI_RSVD_15_8_MASK = 64'h000000000000FF00;
parameter string VSTOPI_RSVD_15_8_SW_TYPE = "WARL";

parameter int VSTOPI_IPRIO_MSB = 7;
parameter int VSTOPI_IPRIO_LSB = 0;
parameter int VSTOPI_IPRIO_WIDTH = 8;
parameter logic [7:0] VSTOPI_IPRIO_RESET = 64'h0000000000000000[7:0];
parameter logic [63:0] VSTOPI_IPRIO_MASK = 64'h00000000000000FF;
parameter string VSTOPI_IPRIO_SW_TYPE = "WARL";


// MTINST CSR Field Defines
parameter int MTINST_MTINST_MSB = 63;
parameter int MTINST_MTINST_LSB = 0;
parameter int MTINST_MTINST_WIDTH = 64;
parameter logic [63:0] MTINST_MTINST_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MTINST_MTINST_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MTINST_MTINST_SW_TYPE = "WARL";


// MTVAL2 CSR Field Defines
parameter int MTVAL2_MTVAL2_MSB = 63;
parameter int MTVAL2_MTVAL2_LSB = 0;
parameter int MTVAL2_MTVAL2_WIDTH = 64;
parameter logic [63:0] MTVAL2_MTVAL2_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MTVAL2_MTVAL2_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MTVAL2_MTVAL2_SW_TYPE = "WARL";


// SCOUNTOVF CSR Field Defines
parameter int SCOUNTOVF_SCOUNTOVF_MSB = 31;
parameter int SCOUNTOVF_SCOUNTOVF_LSB = 0;
parameter int SCOUNTOVF_SCOUNTOVF_WIDTH = 32;
parameter logic [31:0] SCOUNTOVF_SCOUNTOVF_RESET = 64'h0000000000000000[31:0];
parameter logic [63:0] SCOUNTOVF_SCOUNTOVF_MASK = 64'h00000000FFFFFFFF;
parameter string SCOUNTOVF_SCOUNTOVF_SW_TYPE = "WARL";


// MNSTATUS CSR Field Defines
parameter int MNSTATUS_MNPP_MSB = 12;
parameter int MNSTATUS_MNPP_LSB = 11;
parameter int MNSTATUS_MNPP_WIDTH = 2;
parameter logic [1:0] MNSTATUS_MNPP_RESET = 64'h0000000000000000[12:11];
parameter logic [63:0] MNSTATUS_MNPP_MASK = 64'h0000000000001800;
parameter string MNSTATUS_MNPP_SW_TYPE = "WARL";

parameter int MNSTATUS_MNPV_MSB = 7;
parameter int MNSTATUS_MNPV_LSB = 7;
parameter int MNSTATUS_MNPV_WIDTH = 1;
parameter logic [0:0] MNSTATUS_MNPV_RESET = 64'h0000000000000000[7:7];
parameter logic [63:0] MNSTATUS_MNPV_MASK = 64'h0000000000000080;
parameter string MNSTATUS_MNPV_SW_TYPE = "WARL";

parameter int MNSTATUS_NMIE_MSB = 3;
parameter int MNSTATUS_NMIE_LSB = 3;
parameter int MNSTATUS_NMIE_WIDTH = 1;
parameter logic [0:0] MNSTATUS_NMIE_RESET = 64'h0000000000000000[3:3];
parameter logic [63:0] MNSTATUS_NMIE_MASK = 64'h0000000000000008;
parameter string MNSTATUS_NMIE_SW_TYPE = "WARL";


// MNSCRATCH CSR Field Defines
parameter int MNSCRATCH_MNSCRATCH_MSB = 63;
parameter int MNSCRATCH_MNSCRATCH_LSB = 0;
parameter int MNSCRATCH_MNSCRATCH_WIDTH = 64;
parameter logic [63:0] MNSCRATCH_MNSCRATCH_RESET = 64'h0000000000000000[63:0];
parameter logic [63:0] MNSCRATCH_MNSCRATCH_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string MNSCRATCH_MNSCRATCH_SW_TYPE = "WARL";


// MNEPC CSR Field Defines
parameter int MNEPC_ADDR_MSB = 63;
parameter int MNEPC_ADDR_LSB = 1;
parameter int MNEPC_ADDR_WIDTH = 63;
parameter logic [62:0] MNEPC_ADDR_RESET = 64'h0000000000000000[63:1];
parameter logic [63:0] MNEPC_ADDR_MASK = 64'hFFFFFFFFFFFFFFFE;
parameter string MNEPC_ADDR_SW_TYPE = "WARL";


// MNCAUSE CSR Field Defines
parameter int MNCAUSE_INTERRUPT_MSB = 63;
parameter int MNCAUSE_INTERRUPT_LSB = 63;
parameter int MNCAUSE_INTERRUPT_WIDTH = 1;
parameter logic [0:0] MNCAUSE_INTERRUPT_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MNCAUSE_INTERRUPT_MASK = 64'h8000000000000000;
parameter string MNCAUSE_INTERRUPT_SW_TYPE = "WARL";

parameter int MNCAUSE_EXCEPTIONCODE_MSB = 62;
parameter int MNCAUSE_EXCEPTIONCODE_LSB = 0;
parameter int MNCAUSE_EXCEPTIONCODE_WIDTH = 63;
parameter logic [62:0] MNCAUSE_EXCEPTIONCODE_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] MNCAUSE_EXCEPTIONCODE_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string MNCAUSE_EXCEPTIONCODE_SW_TYPE = "WARL";


// MSTATEEN0 CSR Field Defines
parameter int MSTATEEN0_SE0_MSB = 63;
parameter int MSTATEEN0_SE0_LSB = 63;
parameter int MSTATEEN0_SE0_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_SE0_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MSTATEEN0_SE0_MASK = 64'h8000000000000000;
parameter string MSTATEEN0_SE0_SW_TYPE = "WARL";

parameter int MSTATEEN0_ENVCFG_MSB = 62;
parameter int MSTATEEN0_ENVCFG_LSB = 62;
parameter int MSTATEEN0_ENVCFG_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_ENVCFG_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] MSTATEEN0_ENVCFG_MASK = 64'h4000000000000000;
parameter string MSTATEEN0_ENVCFG_SW_TYPE = "WARL";

parameter int MSTATEEN0_WPRI_1_MSB = 61;
parameter int MSTATEEN0_WPRI_1_LSB = 61;
parameter int MSTATEEN0_WPRI_1_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_WPRI_1_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] MSTATEEN0_WPRI_1_MASK = 64'h2000000000000000;
parameter string MSTATEEN0_WPRI_1_SW_TYPE = "WPRI";

parameter int MSTATEEN0_CSRIND_MSB = 60;
parameter int MSTATEEN0_CSRIND_LSB = 60;
parameter int MSTATEEN0_CSRIND_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_CSRIND_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] MSTATEEN0_CSRIND_MASK = 64'h1000000000000000;
parameter string MSTATEEN0_CSRIND_SW_TYPE = "WARL";

parameter int MSTATEEN0_AIA_MSB = 59;
parameter int MSTATEEN0_AIA_LSB = 59;
parameter int MSTATEEN0_AIA_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_AIA_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] MSTATEEN0_AIA_MASK = 64'h0800000000000000;
parameter string MSTATEEN0_AIA_SW_TYPE = "WARL";

parameter int MSTATEEN0_IMSIC_MSB = 58;
parameter int MSTATEEN0_IMSIC_LSB = 58;
parameter int MSTATEEN0_IMSIC_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_IMSIC_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] MSTATEEN0_IMSIC_MASK = 64'h0400000000000000;
parameter string MSTATEEN0_IMSIC_SW_TYPE = "WARL";

parameter int MSTATEEN0_CONTEXT_MSB = 57;
parameter int MSTATEEN0_CONTEXT_LSB = 57;
parameter int MSTATEEN0_CONTEXT_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_CONTEXT_RESET = 64'h0000000000000000[57:57];
parameter logic [63:0] MSTATEEN0_CONTEXT_MASK = 64'h0200000000000000;
parameter string MSTATEEN0_CONTEXT_SW_TYPE = "WARL";

parameter int MSTATEEN0_P1P13_MSB = 56;
parameter int MSTATEEN0_P1P13_LSB = 56;
parameter int MSTATEEN0_P1P13_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_P1P13_RESET = 64'h0000000000000000[56:56];
parameter logic [63:0] MSTATEEN0_P1P13_MASK = 64'h0100000000000000;
parameter string MSTATEEN0_P1P13_SW_TYPE = "WARL";

parameter int MSTATEEN0_SRMCFG_MSB = 55;
parameter int MSTATEEN0_SRMCFG_LSB = 55;
parameter int MSTATEEN0_SRMCFG_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_SRMCFG_RESET = 64'h0000000000000000[55:55];
parameter logic [63:0] MSTATEEN0_SRMCFG_MASK = 64'h0080000000000000;
parameter string MSTATEEN0_SRMCFG_SW_TYPE = "WARL";

parameter int MSTATEEN0_WPRI_0_MSB = 54;
parameter int MSTATEEN0_WPRI_0_LSB = 3;
parameter int MSTATEEN0_WPRI_0_WIDTH = 52;
parameter logic [51:0] MSTATEEN0_WPRI_0_RESET = 64'h0000000000000000[54:3];
parameter logic [63:0] MSTATEEN0_WPRI_0_MASK = 64'h007FFFFFFFFFFFF8;
parameter string MSTATEEN0_WPRI_0_SW_TYPE = "WPRI";

parameter int MSTATEEN0_JVT_MSB = 2;
parameter int MSTATEEN0_JVT_LSB = 2;
parameter int MSTATEEN0_JVT_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_JVT_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] MSTATEEN0_JVT_MASK = 64'h0000000000000004;
parameter string MSTATEEN0_JVT_SW_TYPE = "WARL";

parameter int MSTATEEN0_FCSR_MSB = 1;
parameter int MSTATEEN0_FCSR_LSB = 1;
parameter int MSTATEEN0_FCSR_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_FCSR_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] MSTATEEN0_FCSR_MASK = 64'h0000000000000002;
parameter string MSTATEEN0_FCSR_SW_TYPE = "WARL";

parameter int MSTATEEN0_C_MSB = 0;
parameter int MSTATEEN0_C_LSB = 0;
parameter int MSTATEEN0_C_WIDTH = 1;
parameter logic [0:0] MSTATEEN0_C_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] MSTATEEN0_C_MASK = 64'h0000000000000001;
parameter string MSTATEEN0_C_SW_TYPE = "WARL";


// MSTATEEN1 CSR Field Defines
parameter int MSTATEEN1_SE1_MSB = 63;
parameter int MSTATEEN1_SE1_LSB = 63;
parameter int MSTATEEN1_SE1_WIDTH = 1;
parameter logic [0:0] MSTATEEN1_SE1_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MSTATEEN1_SE1_MASK = 64'h8000000000000000;
parameter string MSTATEEN1_SE1_SW_TYPE = "WARL";

parameter int MSTATEEN1_WPRI_MSB = 62;
parameter int MSTATEEN1_WPRI_LSB = 0;
parameter int MSTATEEN1_WPRI_WIDTH = 63;
parameter logic [62:0] MSTATEEN1_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] MSTATEEN1_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string MSTATEEN1_WPRI_SW_TYPE = "WPRI";


// MSTATEEN2 CSR Field Defines
parameter int MSTATEEN2_SE2_MSB = 63;
parameter int MSTATEEN2_SE2_LSB = 63;
parameter int MSTATEEN2_SE2_WIDTH = 1;
parameter logic [0:0] MSTATEEN2_SE2_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MSTATEEN2_SE2_MASK = 64'h8000000000000000;
parameter string MSTATEEN2_SE2_SW_TYPE = "WARL";

parameter int MSTATEEN2_WPRI_MSB = 62;
parameter int MSTATEEN2_WPRI_LSB = 0;
parameter int MSTATEEN2_WPRI_WIDTH = 63;
parameter logic [62:0] MSTATEEN2_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] MSTATEEN2_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string MSTATEEN2_WPRI_SW_TYPE = "WPRI";


// MSTATEEN3 CSR Field Defines
parameter int MSTATEEN3_SE3_MSB = 63;
parameter int MSTATEEN3_SE3_LSB = 63;
parameter int MSTATEEN3_SE3_WIDTH = 1;
parameter logic [0:0] MSTATEEN3_SE3_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] MSTATEEN3_SE3_MASK = 64'h8000000000000000;
parameter string MSTATEEN3_SE3_SW_TYPE = "WARL";

parameter int MSTATEEN3_WPRI_MSB = 62;
parameter int MSTATEEN3_WPRI_LSB = 0;
parameter int MSTATEEN3_WPRI_WIDTH = 63;
parameter logic [62:0] MSTATEEN3_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] MSTATEEN3_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string MSTATEEN3_WPRI_SW_TYPE = "WPRI";


// HSTATEEN0 CSR Field Defines
parameter int HSTATEEN0_SE0_MSB = 63;
parameter int HSTATEEN0_SE0_LSB = 63;
parameter int HSTATEEN0_SE0_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_SE0_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] HSTATEEN0_SE0_MASK = 64'h8000000000000000;
parameter string HSTATEEN0_SE0_SW_TYPE = "WARL";

parameter int HSTATEEN0_ENVCFG_MSB = 62;
parameter int HSTATEEN0_ENVCFG_LSB = 62;
parameter int HSTATEEN0_ENVCFG_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_ENVCFG_RESET = 64'h0000000000000000[62:62];
parameter logic [63:0] HSTATEEN0_ENVCFG_MASK = 64'h4000000000000000;
parameter string HSTATEEN0_ENVCFG_SW_TYPE = "WARL";

parameter int HSTATEEN0_WPRI_1_MSB = 61;
parameter int HSTATEEN0_WPRI_1_LSB = 61;
parameter int HSTATEEN0_WPRI_1_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_WPRI_1_RESET = 64'h0000000000000000[61:61];
parameter logic [63:0] HSTATEEN0_WPRI_1_MASK = 64'h2000000000000000;
parameter string HSTATEEN0_WPRI_1_SW_TYPE = "WPRI";

parameter int HSTATEEN0_CSRIND_MSB = 60;
parameter int HSTATEEN0_CSRIND_LSB = 60;
parameter int HSTATEEN0_CSRIND_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_CSRIND_RESET = 64'h0000000000000000[60:60];
parameter logic [63:0] HSTATEEN0_CSRIND_MASK = 64'h1000000000000000;
parameter string HSTATEEN0_CSRIND_SW_TYPE = "WARL";

parameter int HSTATEEN0_AIA_MSB = 59;
parameter int HSTATEEN0_AIA_LSB = 59;
parameter int HSTATEEN0_AIA_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_AIA_RESET = 64'h0000000000000000[59:59];
parameter logic [63:0] HSTATEEN0_AIA_MASK = 64'h0800000000000000;
parameter string HSTATEEN0_AIA_SW_TYPE = "WARL";

parameter int HSTATEEN0_IMSIC_MSB = 58;
parameter int HSTATEEN0_IMSIC_LSB = 58;
parameter int HSTATEEN0_IMSIC_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_IMSIC_RESET = 64'h0000000000000000[58:58];
parameter logic [63:0] HSTATEEN0_IMSIC_MASK = 64'h0400000000000000;
parameter string HSTATEEN0_IMSIC_SW_TYPE = "WARL";

parameter int HSTATEEN0_CONTEXT_MSB = 57;
parameter int HSTATEEN0_CONTEXT_LSB = 57;
parameter int HSTATEEN0_CONTEXT_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_CONTEXT_RESET = 64'h0000000000000000[57:57];
parameter logic [63:0] HSTATEEN0_CONTEXT_MASK = 64'h0200000000000000;
parameter string HSTATEEN0_CONTEXT_SW_TYPE = "WARL";

parameter int HSTATEEN0_WPRI_0_MSB = 56;
parameter int HSTATEEN0_WPRI_0_LSB = 3;
parameter int HSTATEEN0_WPRI_0_WIDTH = 54;
parameter logic [53:0] HSTATEEN0_WPRI_0_RESET = 64'h0000000000000000[56:3];
parameter logic [63:0] HSTATEEN0_WPRI_0_MASK = 64'h01FFFFFFFFFFFFF8;
parameter string HSTATEEN0_WPRI_0_SW_TYPE = "WPRI";

parameter int HSTATEEN0_JVT_MSB = 2;
parameter int HSTATEEN0_JVT_LSB = 2;
parameter int HSTATEEN0_JVT_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_JVT_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] HSTATEEN0_JVT_MASK = 64'h0000000000000004;
parameter string HSTATEEN0_JVT_SW_TYPE = "WARL";

parameter int HSTATEEN0_FCSR_MSB = 1;
parameter int HSTATEEN0_FCSR_LSB = 1;
parameter int HSTATEEN0_FCSR_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_FCSR_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] HSTATEEN0_FCSR_MASK = 64'h0000000000000002;
parameter string HSTATEEN0_FCSR_SW_TYPE = "WARL";

parameter int HSTATEEN0_C_MSB = 0;
parameter int HSTATEEN0_C_LSB = 0;
parameter int HSTATEEN0_C_WIDTH = 1;
parameter logic [0:0] HSTATEEN0_C_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] HSTATEEN0_C_MASK = 64'h0000000000000001;
parameter string HSTATEEN0_C_SW_TYPE = "WARL";


// HSTATEEN1 CSR Field Defines
parameter int HSTATEEN1_SE1_MSB = 63;
parameter int HSTATEEN1_SE1_LSB = 63;
parameter int HSTATEEN1_SE1_WIDTH = 1;
parameter logic [0:0] HSTATEEN1_SE1_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] HSTATEEN1_SE1_MASK = 64'h8000000000000000;
parameter string HSTATEEN1_SE1_SW_TYPE = "WARL";

parameter int HSTATEEN1_WPRI_MSB = 62;
parameter int HSTATEEN1_WPRI_LSB = 0;
parameter int HSTATEEN1_WPRI_WIDTH = 63;
parameter logic [62:0] HSTATEEN1_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] HSTATEEN1_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string HSTATEEN1_WPRI_SW_TYPE = "WPRI";


// HSTATEEN2 CSR Field Defines
parameter int HSTATEEN2_SE2_MSB = 63;
parameter int HSTATEEN2_SE2_LSB = 63;
parameter int HSTATEEN2_SE2_WIDTH = 1;
parameter logic [0:0] HSTATEEN2_SE2_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] HSTATEEN2_SE2_MASK = 64'h8000000000000000;
parameter string HSTATEEN2_SE2_SW_TYPE = "WARL";

parameter int HSTATEEN2_WPRI_MSB = 62;
parameter int HSTATEEN2_WPRI_LSB = 0;
parameter int HSTATEEN2_WPRI_WIDTH = 63;
parameter logic [62:0] HSTATEEN2_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] HSTATEEN2_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string HSTATEEN2_WPRI_SW_TYPE = "WPRI";


// HSTATEEN3 CSR Field Defines
parameter int HSTATEEN3_SE3_MSB = 63;
parameter int HSTATEEN3_SE3_LSB = 63;
parameter int HSTATEEN3_SE3_WIDTH = 1;
parameter logic [0:0] HSTATEEN3_SE3_RESET = 64'h0000000000000000[63:63];
parameter logic [63:0] HSTATEEN3_SE3_MASK = 64'h8000000000000000;
parameter string HSTATEEN3_SE3_SW_TYPE = "WARL";

parameter int HSTATEEN3_WPRI_MSB = 62;
parameter int HSTATEEN3_WPRI_LSB = 0;
parameter int HSTATEEN3_WPRI_WIDTH = 63;
parameter logic [62:0] HSTATEEN3_WPRI_RESET = 64'h0000000000000000[62:0];
parameter logic [63:0] HSTATEEN3_WPRI_MASK = 64'h7FFFFFFFFFFFFFFF;
parameter string HSTATEEN3_WPRI_SW_TYPE = "WPRI";


// SSTATEEN0 CSR Field Defines
parameter int SSTATEEN0_WPRI_MSB = 31;
parameter int SSTATEEN0_WPRI_LSB = 3;
parameter int SSTATEEN0_WPRI_WIDTH = 29;
parameter logic [28:0] SSTATEEN0_WPRI_RESET = 64'h0000000000000000[31:3];
parameter logic [63:0] SSTATEEN0_WPRI_MASK = 64'h00000000FFFFFFF8;
parameter string SSTATEEN0_WPRI_SW_TYPE = "WPRI";

parameter int SSTATEEN0_JVT_MSB = 2;
parameter int SSTATEEN0_JVT_LSB = 2;
parameter int SSTATEEN0_JVT_WIDTH = 1;
parameter logic [0:0] SSTATEEN0_JVT_RESET = 64'h0000000000000000[2:2];
parameter logic [63:0] SSTATEEN0_JVT_MASK = 64'h0000000000000004;
parameter string SSTATEEN0_JVT_SW_TYPE = "WARL";

parameter int SSTATEEN0_FCSR_MSB = 1;
parameter int SSTATEEN0_FCSR_LSB = 1;
parameter int SSTATEEN0_FCSR_WIDTH = 1;
parameter logic [0:0] SSTATEEN0_FCSR_RESET = 64'h0000000000000000[1:1];
parameter logic [63:0] SSTATEEN0_FCSR_MASK = 64'h0000000000000002;
parameter string SSTATEEN0_FCSR_SW_TYPE = "WARL";

parameter int SSTATEEN0_C_MSB = 0;
parameter int SSTATEEN0_C_LSB = 0;
parameter int SSTATEEN0_C_WIDTH = 1;
parameter logic [0:0] SSTATEEN0_C_RESET = 64'h0000000000000000[0:0];
parameter logic [63:0] SSTATEEN0_C_MASK = 64'h0000000000000001;
parameter string SSTATEEN0_C_SW_TYPE = "WARL";


// SSTATEEN1 CSR Field Defines
parameter int SSTATEEN1_WPRI_MSB = 31;
parameter int SSTATEEN1_WPRI_LSB = 0;
parameter int SSTATEEN1_WPRI_WIDTH = 32;
parameter logic [31:0] SSTATEEN1_WPRI_RESET = 64'h0000000000000000[31:0];
parameter logic [63:0] SSTATEEN1_WPRI_MASK = 64'h00000000FFFFFFFF;
parameter string SSTATEEN1_WPRI_SW_TYPE = "WPRI";


// SSTATEEN2 CSR Field Defines
parameter int SSTATEEN2_WPRI_MSB = 31;
parameter int SSTATEEN2_WPRI_LSB = 0;
parameter int SSTATEEN2_WPRI_WIDTH = 32;
parameter logic [31:0] SSTATEEN2_WPRI_RESET = 64'h0000000000000000[31:0];
parameter logic [63:0] SSTATEEN2_WPRI_MASK = 64'h00000000FFFFFFFF;
parameter string SSTATEEN2_WPRI_SW_TYPE = "WPRI";


// SSTATEEN3 CSR Field Defines
parameter int SSTATEEN3_WPRI_MSB = 31;
parameter int SSTATEEN3_WPRI_LSB = 0;
parameter int SSTATEEN3_WPRI_WIDTH = 32;
parameter logic [31:0] SSTATEEN3_WPRI_RESET = 64'h0000000000000000[31:0];
parameter logic [63:0] SSTATEEN3_WPRI_MASK = 64'h00000000FFFFFFFF;
parameter string SSTATEEN3_WPRI_SW_TYPE = "WPRI";


// CSR Reset Value Defines
parameter logic [63:0] CYCLE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] TIME_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] INSTRET_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER3_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER4_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER5_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER6_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER7_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER8_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER9_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER10_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER11_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER12_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER13_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER14_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER15_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER16_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER17_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER18_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER19_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER20_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER21_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER22_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER23_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER24_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER25_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER26_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER27_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER28_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER29_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER30_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HPMCOUNTER31_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MISA_RESET_VAL = 64'h80000000003411AF;
parameter logic [63:0] MSTATUS_RESET_VAL = 64'h0000000A00000000;
parameter logic [63:0] MTVEC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MEDELEG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MIDELEG_RESET_VAL = 64'h0000000000001444;
parameter logic [63:0] MIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MIE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSCRATCH_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MEPC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MCAUSE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MTVAL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MCONFIGPTR_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MENVCFG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSECCFG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MCYCLE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MINSTRET_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER3_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER4_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER5_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER6_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER7_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER8_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER9_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER10_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER11_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER12_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER13_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER14_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER15_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER16_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER17_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER18_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER19_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER20_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER21_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER22_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER23_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER24_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER25_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER26_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER27_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER28_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER29_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER30_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMCOUNTER31_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT3_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT4_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT5_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT6_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT7_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT8_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT9_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT10_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT11_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT12_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT13_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT14_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT15_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT16_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT17_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT18_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT19_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT20_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT21_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT22_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT23_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT24_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT25_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT26_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT27_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT28_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT29_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT30_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MHPMEVENT31_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MCOUNTEREN_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MCOUNTINHIBIT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MISELECT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MIREG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MTOPEI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MTOPI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MVIEN_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MVIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSTATUS_RESET_VAL = 64'h0000000200000000;
parameter logic [63:0] STVEC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SIE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SCOUNTEREN_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSCRATCH_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SEPC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SCAUSE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] STVAL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] STIMECMP_RESET_VAL = 64'h00000000FFFFFFFF;
parameter logic [63:0] SENVCFG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SATP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SRMCFG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SISELECT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SIREG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] STOPEI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] STOPI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SEED_RESET_VAL = 64'h0000000040000000;
parameter logic [63:0] FFLAGS_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] FRM_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] FCSR_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTART_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VXSAT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VXRM_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VCSR_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VTYPE_RESET_VAL = 64'h8000000000000000;
parameter logic [63:0] VLENB_RESET_VAL = 64'h0000000000000020;
parameter logic [63:0] PMPCFG0_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] PMPCFG2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] PMPADDR0_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR1_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR2_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR3_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR4_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR5_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR6_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR7_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR8_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR9_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR10_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR11_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR12_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR13_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR14_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] PMPADDR15_RESET_VAL = 64'h00000000000001FF;
parameter logic [63:0] TSELECT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] DCSR_RESET_VAL = 64'h0000000000000003;
parameter logic [63:0] DPC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] DSCRATCH0_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] DSCRATCH1_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HSTATUS_RESET_VAL = 64'h0000000200000000;
parameter logic [63:0] HEDELEG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HIDELEG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HVIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HVIPRIO1_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HVIPRIO2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HIE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HGEIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HGEIE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HENVCFG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HCOUNTEREN_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HTIMEDELTA_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HTVAL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HTINST_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HGATP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HVIEN_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HVICTL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSSTATUS_RESET_VAL = 64'h0000000200000000;
parameter logic [63:0] VSIP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSIE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTVEC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSSCRATCH_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSEPC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSCAUSE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTVAL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTIMECMP_RESET_VAL = 64'h00000000FFFFFFFF;
parameter logic [63:0] VSATP_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSISELECT_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSIREG_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTOPEI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] VSTOPI_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MTINST_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MTVAL2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SCOUNTOVF_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MNSTATUS_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MNSCRATCH_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MNEPC_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MNCAUSE_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSTATEEN0_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSTATEEN1_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSTATEEN2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MSTATEEN3_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HSTATEEN0_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HSTATEEN1_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HSTATEEN2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] HSTATEEN3_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSTATEEN0_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSTATEEN1_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSTATEEN2_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] SSTATEEN3_RESET_VAL = 64'h0000000000000000;

// Utility Macros for Field Access
// Extract field value from CSR value
`define CSR_FIELD_GET(csr_val, field_msb, field_lsb) \
    ((csr_val >> field_lsb) & ((1 << (field_msb - field_lsb + 1)) - 1))

// Set field value in CSR value
`define CSR_FIELD_SET(csr_val, field_val, field_msb, field_lsb) \
    ((csr_val & ~(((1 << (field_msb - field_lsb + 1)) - 1) << field_lsb)) | \
     ((field_val & ((1 << (field_msb - field_lsb + 1)) - 1)) << field_lsb))

// Field Access Macros
`define CYCLE_CYCLE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define CYCLE_CYCLE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define TIME_TIME_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define TIME_TIME_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define INSTRET_INSTRET_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define INSTRET_INSTRET_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER3_HPMCOUNTER3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER3_HPMCOUNTER3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER4_HPMCOUNTER4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER4_HPMCOUNTER4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER5_HPMCOUNTER5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER5_HPMCOUNTER5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER6_HPMCOUNTER6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER6_HPMCOUNTER6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER7_HPMCOUNTER7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER7_HPMCOUNTER7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER8_HPMCOUNTER8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER8_HPMCOUNTER8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER9_HPMCOUNTER9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER9_HPMCOUNTER9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER10_HPMCOUNTER10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER10_HPMCOUNTER10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER11_HPMCOUNTER11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER11_HPMCOUNTER11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER12_HPMCOUNTER12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER12_HPMCOUNTER12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER13_HPMCOUNTER13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER13_HPMCOUNTER13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER14_HPMCOUNTER14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER14_HPMCOUNTER14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER15_HPMCOUNTER15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER15_HPMCOUNTER15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER16_HPMCOUNTER16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER16_HPMCOUNTER16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER17_HPMCOUNTER17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER17_HPMCOUNTER17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER18_HPMCOUNTER18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER18_HPMCOUNTER18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER19_HPMCOUNTER19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER19_HPMCOUNTER19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER20_HPMCOUNTER20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER20_HPMCOUNTER20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER21_HPMCOUNTER21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER21_HPMCOUNTER21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER22_HPMCOUNTER22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER22_HPMCOUNTER22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER23_HPMCOUNTER23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER23_HPMCOUNTER23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER24_HPMCOUNTER24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER24_HPMCOUNTER24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER25_HPMCOUNTER25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER25_HPMCOUNTER25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER26_HPMCOUNTER26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER26_HPMCOUNTER26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER27_HPMCOUNTER27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER27_HPMCOUNTER27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER28_HPMCOUNTER28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER28_HPMCOUNTER28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER29_HPMCOUNTER29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER29_HPMCOUNTER29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER30_HPMCOUNTER30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER30_HPMCOUNTER30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HPMCOUNTER31_HPMCOUNTER31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HPMCOUNTER31_HPMCOUNTER31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MISA_MXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 62)

`define MISA_MXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 62)

`define MISA_WLRL0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 26)

`define MISA_WLRL0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 26)

`define MISA_Z_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 25)

`define MISA_Z_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 25)

`define MISA_Y_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 24, 24)

`define MISA_Y_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 24, 24)

`define MISA_X_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define MISA_X_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define MISA_W_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define MISA_W_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define MISA_V_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define MISA_V_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define MISA_U_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define MISA_U_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define MISA_T_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define MISA_T_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define MISA_S_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define MISA_S_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define MISA_R_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define MISA_R_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define MISA_Q_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define MISA_Q_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define MISA_P_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define MISA_P_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define MISA_O_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define MISA_O_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define MISA_N_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MISA_N_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MISA_M_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MISA_M_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MISA_L_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MISA_L_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MISA_K_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MISA_K_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MISA_J_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MISA_J_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MISA_I_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define MISA_I_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define MISA_H_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MISA_H_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MISA_G_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MISA_G_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MISA_F_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MISA_F_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MISA_E_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define MISA_E_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define MISA_D_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MISA_D_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MISA_C_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MISA_C_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MISA_B_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MISA_B_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MISA_A_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MISA_A_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MSTATUS_SD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MSTATUS_SD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MSTATUS_MSTATUS_WPRI_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 40)

`define MSTATUS_MSTATUS_WPRI_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 40)

`define MSTATUS_MPV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 39, 39)

`define MSTATUS_MPV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 39, 39)

`define MSTATUS_GVA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 38, 38)

`define MSTATUS_GVA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 38, 38)

`define MSTATUS_MBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 37, 37)

`define MSTATUS_MBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 37, 37)

`define MSTATUS_SBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 36, 36)

`define MSTATUS_SBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 36, 36)

`define MSTATUS_SXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 35, 34)

`define MSTATUS_SXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 35, 34)

`define MSTATUS_UXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define MSTATUS_UXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define MSTATUS_MSTATUS_WPRI_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 23)

`define MSTATUS_MSTATUS_WPRI_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 23)

`define MSTATUS_TSR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define MSTATUS_TSR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define MSTATUS_TW_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define MSTATUS_TW_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define MSTATUS_TVM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define MSTATUS_TVM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define MSTATUS_MXR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define MSTATUS_MXR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define MSTATUS_SUM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define MSTATUS_SUM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define MSTATUS_MPRV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define MSTATUS_MPRV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define MSTATUS_XS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 15)

`define MSTATUS_XS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 15)

`define MSTATUS_FS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 13)

`define MSTATUS_FS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 13)

`define MSTATUS_MPP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define MSTATUS_MPP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define MSTATUS_VS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 9)

`define MSTATUS_VS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 9)

`define MSTATUS_SPP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define MSTATUS_SPP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define MSTATUS_MPIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MSTATUS_MPIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MSTATUS_UBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MSTATUS_UBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MSTATUS_SPIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MSTATUS_SPIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MSTATUS_MSTATUS_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define MSTATUS_MSTATUS_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define MSTATUS_MIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MSTATUS_MIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MSTATUS_MSTATUS_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MSTATUS_MSTATUS_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MSTATUS_SIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MSTATUS_SIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MSTATUS_MSTATUS_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MSTATUS_MSTATUS_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MTVEC_BASE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 2)

`define MTVEC_BASE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 2)

`define MTVEC_MODE_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MTVEC_MODE_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MTVEC_MODE_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MTVEC_MODE_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MEDELEG_RSVD_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 24)

`define MEDELEG_RSVD_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 24)

`define MEDELEG_MEDELEG_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 20)

`define MEDELEG_MEDELEG_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 20)

`define MEDELEG_RSVD_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 16)

`define MEDELEG_RSVD_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 16)

`define MEDELEG_MEDELEG_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define MEDELEG_MEDELEG_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define MEDELEG_RSVD_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define MEDELEG_RSVD_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define MEDELEG_MEDELEG_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 12)

`define MEDELEG_MEDELEG_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 12)

`define MEDELEG_ECALL_FROM_M_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MEDELEG_ECALL_FROM_M_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MEDELEG_MEDELEG_MASKED_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MEDELEG_MEDELEG_MASKED_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MEDELEG_MEDELEG_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 0)

`define MEDELEG_MEDELEG_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 0)

`define MIDELEG_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MIDELEG_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MIDELEG_SGEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MIDELEG_SGEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MIDELEG_MEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MIDELEG_MEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MIDELEG_VSEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MIDELEG_VSEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MIDELEG_SEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MIDELEG_SEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MIDELEG_MTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MIDELEG_MTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MIDELEG_VSTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MIDELEG_VSTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MIDELEG_STIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MIDELEG_STIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MIDELEG_MSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MIDELEG_MSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MIDELEG_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MIDELEG_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MIDELEG_SSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MIDELEG_SSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MIP_NONSTANDARDINTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 16)

`define MIP_NONSTANDARDINTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 16)

`define MIP_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MIP_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MIP_SGEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MIP_SGEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MIP_MEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MIP_MEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MIP_VSEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MIP_VSEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MIP_SEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MIP_SEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MIP_MTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MIP_MTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MIP_VSTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MIP_VSTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MIP_STIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MIP_STIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MIP_MSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MIP_MSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MIP_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MIP_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MIP_SSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MIP_SSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MIE_NONSTANDARDINTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 16)

`define MIE_NONSTANDARDINTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 16)

`define MIE_LCOFIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MIE_LCOFIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MIE_SGEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MIE_SGEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MIE_MEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MIE_MEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MIE_VSEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MIE_VSEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MIE_SEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MIE_SEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MIE_MTIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MIE_MTIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MIE_VSTIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MIE_VSTIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MIE_STIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MIE_STIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MIE_MSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MIE_MSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MIE_VSSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MIE_VSSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MIE_SSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MIE_SSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MSCRATCH_MSCRATCH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MSCRATCH_MSCRATCH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MEPC_ADDR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 1)

`define MEPC_ADDR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 1)

`define MCAUSE_INTERRUPT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MCAUSE_INTERRUPT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MCAUSE_EXCEPTIONCODEWLRL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define MCAUSE_EXCEPTIONCODEWLRL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define MTVAL_MTVAL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MTVAL_MTVAL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MCONFIGPTR_MCONFIGPTR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MCONFIGPTR_MCONFIGPTR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MENVCFG_STCE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MENVCFG_STCE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MENVCFG_PBMTE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MENVCFG_PBMTE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MENVCFG_HADE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MENVCFG_HADE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MENVCFG_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 34)

`define MENVCFG_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 34)

`define MENVCFG_PMM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define MENVCFG_PMM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define MENVCFG_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 8)

`define MENVCFG_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 8)

`define MENVCFG_CBZE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MENVCFG_CBZE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MENVCFG_CBCFE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MENVCFG_CBCFE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MENVCFG_CBIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 4)

`define MENVCFG_CBIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 4)

`define MENVCFG_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 1)

`define MENVCFG_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 1)

`define MENVCFG_FIOM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MENVCFG_FIOM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MSECCFG_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 34)

`define MSECCFG_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 34)

`define MSECCFG_PMM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define MSECCFG_PMM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define MSECCFG_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 10)

`define MSECCFG_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 10)

`define MSECCFG_SSEED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MSECCFG_SSEED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MSECCFG_USEED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define MSECCFG_USEED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define MSECCFG_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 3)

`define MSECCFG_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 3)

`define MSECCFG_RLB_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MSECCFG_RLB_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MSECCFG_MMWP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MSECCFG_MMWP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MSECCFG_MML_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MSECCFG_MML_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MCYCLE_CYCLE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MCYCLE_CYCLE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MINSTRET_INSTRET_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MINSTRET_INSTRET_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER3_HPMCOUNTER3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER3_HPMCOUNTER3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER4_HPMCOUNTER4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER4_HPMCOUNTER4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER5_HPMCOUNTER5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER5_HPMCOUNTER5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER6_HPMCOUNTER6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER6_HPMCOUNTER6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER7_HPMCOUNTER7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER7_HPMCOUNTER7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER8_HPMCOUNTER8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER8_HPMCOUNTER8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER9_HPMCOUNTER9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER9_HPMCOUNTER9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER10_HPMCOUNTER10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER10_HPMCOUNTER10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER11_HPMCOUNTER11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER11_HPMCOUNTER11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER12_HPMCOUNTER12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER12_HPMCOUNTER12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER13_HPMCOUNTER13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER13_HPMCOUNTER13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER14_HPMCOUNTER14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER14_HPMCOUNTER14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER15_HPMCOUNTER15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER15_HPMCOUNTER15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER16_HPMCOUNTER16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER16_HPMCOUNTER16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER17_HPMCOUNTER17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER17_HPMCOUNTER17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER18_HPMCOUNTER18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER18_HPMCOUNTER18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER19_HPMCOUNTER19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER19_HPMCOUNTER19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER20_HPMCOUNTER20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER20_HPMCOUNTER20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER21_HPMCOUNTER21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER21_HPMCOUNTER21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER22_HPMCOUNTER22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER22_HPMCOUNTER22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER23_HPMCOUNTER23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER23_HPMCOUNTER23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER24_HPMCOUNTER24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER24_HPMCOUNTER24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER25_HPMCOUNTER25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER25_HPMCOUNTER25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER26_HPMCOUNTER26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER26_HPMCOUNTER26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER27_HPMCOUNTER27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER27_HPMCOUNTER27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER28_HPMCOUNTER28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER28_HPMCOUNTER28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER29_HPMCOUNTER29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER29_HPMCOUNTER29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER30_HPMCOUNTER30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER30_HPMCOUNTER30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMCOUNTER31_HPMCOUNTER31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMCOUNTER31_HPMCOUNTER31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT3_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT3_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT3_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT3_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT3_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT3_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT3_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT3_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT3_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT3_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT3_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT3_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT3_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT3_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT3_MHPMEVENT3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT3_MHPMEVENT3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT4_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT4_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT4_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT4_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT4_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT4_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT4_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT4_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT4_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT4_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT4_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT4_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT4_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT4_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT4_MHPMEVENT4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT4_MHPMEVENT4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT5_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT5_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT5_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT5_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT5_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT5_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT5_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT5_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT5_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT5_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT5_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT5_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT5_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT5_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT5_MHPMEVENT5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT5_MHPMEVENT5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT6_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT6_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT6_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT6_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT6_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT6_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT6_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT6_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT6_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT6_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT6_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT6_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT6_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT6_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT6_MHPMEVENT6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT6_MHPMEVENT6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT7_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT7_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT7_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT7_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT7_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT7_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT7_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT7_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT7_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT7_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT7_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT7_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT7_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT7_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT7_MHPMEVENT7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT7_MHPMEVENT7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT8_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT8_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT8_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT8_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT8_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT8_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT8_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT8_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT8_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT8_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT8_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT8_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT8_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT8_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT8_MHPMEVENT8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT8_MHPMEVENT8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT9_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT9_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT9_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT9_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT9_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT9_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT9_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT9_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT9_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT9_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT9_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT9_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT9_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT9_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT9_MHPMEVENT9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT9_MHPMEVENT9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT10_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MHPMEVENT10_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MHPMEVENT10_MINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MHPMEVENT10_MINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MHPMEVENT10_SINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MHPMEVENT10_SINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MHPMEVENT10_UINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MHPMEVENT10_UINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MHPMEVENT10_VSINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MHPMEVENT10_VSINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MHPMEVENT10_VUINH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MHPMEVENT10_VUINH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MHPMEVENT10_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 56)

`define MHPMEVENT10_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 56)

`define MHPMEVENT10_MHPMEVENT10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 0)

`define MHPMEVENT10_MHPMEVENT10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 0)

`define MHPMEVENT11_MHPMEVENT11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT11_MHPMEVENT11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT12_MHPMEVENT12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT12_MHPMEVENT12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT13_MHPMEVENT13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT13_MHPMEVENT13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT14_MHPMEVENT14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT14_MHPMEVENT14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT15_MHPMEVENT15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT15_MHPMEVENT15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT16_MHPMEVENT16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT16_MHPMEVENT16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT17_MHPMEVENT17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT17_MHPMEVENT17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT18_MHPMEVENT18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT18_MHPMEVENT18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT19_MHPMEVENT19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT19_MHPMEVENT19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT20_MHPMEVENT20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT20_MHPMEVENT20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT21_MHPMEVENT21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT21_MHPMEVENT21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT22_MHPMEVENT22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT22_MHPMEVENT22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT23_MHPMEVENT23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT23_MHPMEVENT23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT24_MHPMEVENT24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT24_MHPMEVENT24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT25_MHPMEVENT25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT25_MHPMEVENT25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT26_MHPMEVENT26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT26_MHPMEVENT26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT27_MHPMEVENT27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT27_MHPMEVENT27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT28_MHPMEVENT28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT28_MHPMEVENT28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT29_MHPMEVENT29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT29_MHPMEVENT29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT30_MHPMEVENT30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT30_MHPMEVENT30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MHPMEVENT31_MHPMEVENT31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MHPMEVENT31_MHPMEVENT31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MCOUNTEREN_HPM31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define MCOUNTEREN_HPM31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define MCOUNTEREN_HPM30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 30)

`define MCOUNTEREN_HPM30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 30)

`define MCOUNTEREN_HPM29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 29, 29)

`define MCOUNTEREN_HPM29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 29, 29)

`define MCOUNTEREN_HPM28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 28)

`define MCOUNTEREN_HPM28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 28)

`define MCOUNTEREN_HPM27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 27)

`define MCOUNTEREN_HPM27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 27)

`define MCOUNTEREN_HPM26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 26)

`define MCOUNTEREN_HPM26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 26)

`define MCOUNTEREN_HPM25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 25)

`define MCOUNTEREN_HPM25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 25)

`define MCOUNTEREN_HPM24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 24, 24)

`define MCOUNTEREN_HPM24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 24, 24)

`define MCOUNTEREN_HPM23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define MCOUNTEREN_HPM23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define MCOUNTEREN_HPM22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define MCOUNTEREN_HPM22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define MCOUNTEREN_HPM21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define MCOUNTEREN_HPM21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define MCOUNTEREN_HPM20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define MCOUNTEREN_HPM20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define MCOUNTEREN_HPM19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define MCOUNTEREN_HPM19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define MCOUNTEREN_HPM18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define MCOUNTEREN_HPM18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define MCOUNTEREN_HPM17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define MCOUNTEREN_HPM17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define MCOUNTEREN_HPM16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define MCOUNTEREN_HPM16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define MCOUNTEREN_HPM15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define MCOUNTEREN_HPM15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define MCOUNTEREN_HPM14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define MCOUNTEREN_HPM14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define MCOUNTEREN_HPM13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MCOUNTEREN_HPM13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MCOUNTEREN_HPM12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MCOUNTEREN_HPM12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MCOUNTEREN_HPM11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MCOUNTEREN_HPM11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MCOUNTEREN_HPM10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MCOUNTEREN_HPM10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MCOUNTEREN_HPM9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MCOUNTEREN_HPM9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MCOUNTEREN_HPM8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define MCOUNTEREN_HPM8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define MCOUNTEREN_HPM7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MCOUNTEREN_HPM7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MCOUNTEREN_HPM6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MCOUNTEREN_HPM6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MCOUNTEREN_HPM5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MCOUNTEREN_HPM5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MCOUNTEREN_HPM4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define MCOUNTEREN_HPM4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define MCOUNTEREN_HPM3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MCOUNTEREN_HPM3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MCOUNTEREN_IR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MCOUNTEREN_IR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MCOUNTEREN_TM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MCOUNTEREN_TM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MCOUNTEREN_CY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MCOUNTEREN_CY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MCOUNTINHIBIT_HPM31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define MCOUNTINHIBIT_HPM31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define MCOUNTINHIBIT_HPM30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 30)

`define MCOUNTINHIBIT_HPM30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 30)

`define MCOUNTINHIBIT_HPM29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 29, 29)

`define MCOUNTINHIBIT_HPM29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 29, 29)

`define MCOUNTINHIBIT_HPM28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 28)

`define MCOUNTINHIBIT_HPM28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 28)

`define MCOUNTINHIBIT_HPM27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 27)

`define MCOUNTINHIBIT_HPM27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 27)

`define MCOUNTINHIBIT_HPM26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 26)

`define MCOUNTINHIBIT_HPM26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 26)

`define MCOUNTINHIBIT_HPM25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 25)

`define MCOUNTINHIBIT_HPM25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 25)

`define MCOUNTINHIBIT_HPM24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 24, 24)

`define MCOUNTINHIBIT_HPM24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 24, 24)

`define MCOUNTINHIBIT_HPM23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define MCOUNTINHIBIT_HPM23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define MCOUNTINHIBIT_HPM22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define MCOUNTINHIBIT_HPM22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define MCOUNTINHIBIT_HPM21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define MCOUNTINHIBIT_HPM21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define MCOUNTINHIBIT_HPM20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define MCOUNTINHIBIT_HPM20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define MCOUNTINHIBIT_HPM19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define MCOUNTINHIBIT_HPM19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define MCOUNTINHIBIT_HPM18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define MCOUNTINHIBIT_HPM18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define MCOUNTINHIBIT_HPM17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define MCOUNTINHIBIT_HPM17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define MCOUNTINHIBIT_HPM16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define MCOUNTINHIBIT_HPM16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define MCOUNTINHIBIT_HPM15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define MCOUNTINHIBIT_HPM15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define MCOUNTINHIBIT_HPM14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define MCOUNTINHIBIT_HPM14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define MCOUNTINHIBIT_HPM13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MCOUNTINHIBIT_HPM13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MCOUNTINHIBIT_HPM12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define MCOUNTINHIBIT_HPM12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define MCOUNTINHIBIT_HPM11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define MCOUNTINHIBIT_HPM11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define MCOUNTINHIBIT_HPM10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define MCOUNTINHIBIT_HPM10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define MCOUNTINHIBIT_HPM9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MCOUNTINHIBIT_HPM9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MCOUNTINHIBIT_HPM8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define MCOUNTINHIBIT_HPM8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define MCOUNTINHIBIT_HPM7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MCOUNTINHIBIT_HPM7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MCOUNTINHIBIT_HPM6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define MCOUNTINHIBIT_HPM6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define MCOUNTINHIBIT_HPM5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MCOUNTINHIBIT_HPM5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MCOUNTINHIBIT_HPM4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define MCOUNTINHIBIT_HPM4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define MCOUNTINHIBIT_HPM3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MCOUNTINHIBIT_HPM3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MCOUNTINHIBIT_IR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MCOUNTINHIBIT_IR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MCOUNTINHIBIT_HARD0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MCOUNTINHIBIT_HARD0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MCOUNTINHIBIT_CY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MCOUNTINHIBIT_CY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MISELECT_RSVD_63_8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 8)

`define MISELECT_RSVD_63_8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 8)

`define MISELECT_INTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define MISELECT_INTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define MIREG_MIREG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MIREG_MIREG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MTOPEI_RSVD_63_27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 27)

`define MTOPEI_RSVD_63_27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 27)

`define MTOPEI_IDENTITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 16)

`define MTOPEI_IDENTITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 16)

`define MTOPEI_RSVD_15_11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 11)

`define MTOPEI_RSVD_15_11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 11)

`define MTOPEI_PRIORITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 0)

`define MTOPEI_PRIORITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 0)

`define MTOPI_RSVD_63_28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 28)

`define MTOPI_RSVD_63_28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 28)

`define MTOPI_IID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 16)

`define MTOPI_IID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 16)

`define MTOPI_RSVD_15_8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 8)

`define MTOPI_RSVD_15_8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 8)

`define MTOPI_IPRIO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define MTOPI_IPRIO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define MVIEN_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MVIEN_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MVIEN_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define MVIEN_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define MVIEN_SEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MVIEN_SEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MVIEN_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 2)

`define MVIEN_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 2)

`define MVIEN_SSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MVIEN_SSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MVIEN_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MVIEN_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MVIP_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define MVIP_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define MVIP_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define MVIP_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define MVIP_SEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define MVIP_SEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define MVIP_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 6)

`define MVIP_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 6)

`define MVIP_STIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define MVIP_STIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define MVIP_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 2)

`define MVIP_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 2)

`define MVIP_SSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MVIP_SSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MVIP_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MVIP_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define SSTATUS_SD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define SSTATUS_SD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define SSTATUS_SSTATUS_WPRI_6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 34)

`define SSTATUS_SSTATUS_WPRI_6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 34)

`define SSTATUS_UXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define SSTATUS_UXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define SSTATUS_SSTATUS_WPRI_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 20)

`define SSTATUS_SSTATUS_WPRI_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 20)

`define SSTATUS_MXR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define SSTATUS_MXR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define SSTATUS_SUM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define SSTATUS_SUM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define SSTATUS_SSTATUS_WPRI_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define SSTATUS_SSTATUS_WPRI_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define SSTATUS_XS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 15)

`define SSTATUS_XS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 15)

`define SSTATUS_FS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 13)

`define SSTATUS_FS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 13)

`define SSTATUS_SSTATUS_WPRI_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define SSTATUS_SSTATUS_WPRI_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define SSTATUS_VS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 9)

`define SSTATUS_VS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 9)

`define SSTATUS_SPP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define SSTATUS_SPP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define SSTATUS_SSTATUS_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define SSTATUS_SSTATUS_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define SSTATUS_UBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define SSTATUS_UBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define SSTATUS_SPIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define SSTATUS_SPIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define SSTATUS_SSTATUS_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 2)

`define SSTATUS_SSTATUS_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 2)

`define SSTATUS_SIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define SSTATUS_SIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define SSTATUS_SSTATUS_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define SSTATUS_SSTATUS_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define STVEC_BASESXLEN12WARL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 2)

`define STVEC_BASESXLEN12WARL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 2)

`define STVEC_MODE_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define STVEC_MODE_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define STVEC_MODE_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define STVEC_MODE_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define SIP_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define SIP_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define SIP_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define SIP_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define SIP_SEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define SIP_SEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define SIP_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 6)

`define SIP_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 6)

`define SIP_STIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define SIP_STIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define SIP_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 2)

`define SIP_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 2)

`define SIP_SSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define SIP_SSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define SIE_LCOFIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define SIE_LCOFIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define SIE_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define SIE_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define SIE_SEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define SIE_SEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define SIE_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 6)

`define SIE_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 6)

`define SIE_STIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define SIE_STIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define SIE_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 2)

`define SIE_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 2)

`define SIE_SSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define SIE_SSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define SCOUNTEREN_HPM31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define SCOUNTEREN_HPM31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define SCOUNTEREN_HPM30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 30)

`define SCOUNTEREN_HPM30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 30)

`define SCOUNTEREN_HPM29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 29, 29)

`define SCOUNTEREN_HPM29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 29, 29)

`define SCOUNTEREN_HPM28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 28)

`define SCOUNTEREN_HPM28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 28)

`define SCOUNTEREN_HPM27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 27)

`define SCOUNTEREN_HPM27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 27)

`define SCOUNTEREN_HPM26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 26)

`define SCOUNTEREN_HPM26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 26)

`define SCOUNTEREN_HPM25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 25)

`define SCOUNTEREN_HPM25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 25)

`define SCOUNTEREN_HPM24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 24, 24)

`define SCOUNTEREN_HPM24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 24, 24)

`define SCOUNTEREN_HPM23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define SCOUNTEREN_HPM23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define SCOUNTEREN_HPM22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define SCOUNTEREN_HPM22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define SCOUNTEREN_HPM21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define SCOUNTEREN_HPM21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define SCOUNTEREN_HPM20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define SCOUNTEREN_HPM20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define SCOUNTEREN_HPM19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define SCOUNTEREN_HPM19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define SCOUNTEREN_HPM18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define SCOUNTEREN_HPM18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define SCOUNTEREN_HPM17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define SCOUNTEREN_HPM17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define SCOUNTEREN_HPM16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define SCOUNTEREN_HPM16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define SCOUNTEREN_HPM15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define SCOUNTEREN_HPM15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define SCOUNTEREN_HPM14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define SCOUNTEREN_HPM14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define SCOUNTEREN_HPM13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define SCOUNTEREN_HPM13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define SCOUNTEREN_HPM12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define SCOUNTEREN_HPM12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define SCOUNTEREN_HPM11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define SCOUNTEREN_HPM11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define SCOUNTEREN_HPM10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define SCOUNTEREN_HPM10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define SCOUNTEREN_HPM9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define SCOUNTEREN_HPM9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define SCOUNTEREN_HPM8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define SCOUNTEREN_HPM8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define SCOUNTEREN_HPM7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define SCOUNTEREN_HPM7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define SCOUNTEREN_HPM6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define SCOUNTEREN_HPM6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define SCOUNTEREN_HPM5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define SCOUNTEREN_HPM5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define SCOUNTEREN_HPM4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define SCOUNTEREN_HPM4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define SCOUNTEREN_HPM3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define SCOUNTEREN_HPM3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define SCOUNTEREN_IR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define SCOUNTEREN_IR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define SCOUNTEREN_TM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define SCOUNTEREN_TM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define SCOUNTEREN_CY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define SCOUNTEREN_CY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define SSCRATCH_SSCRATCH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define SSCRATCH_SSCRATCH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define SEPC_ADDR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 1)

`define SEPC_ADDR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 1)

`define SCAUSE_INTERRUPT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define SCAUSE_INTERRUPT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define SCAUSE_EXCEPTIONCODEWLRL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define SCAUSE_EXCEPTIONCODEWLRL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define STVAL_STVAL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define STVAL_STVAL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define STIMECMP_STIMECMP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define STIMECMP_STIMECMP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define SENVCFG_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 34)

`define SENVCFG_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 34)

`define SENVCFG_PMM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define SENVCFG_PMM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define SENVCFG_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 8)

`define SENVCFG_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 8)

`define SENVCFG_CBZE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define SENVCFG_CBZE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define SENVCFG_CBCFE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define SENVCFG_CBCFE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define SENVCFG_CBIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 4)

`define SENVCFG_CBIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 4)

`define SENVCFG_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 1)

`define SENVCFG_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 1)

`define SENVCFG_FIOM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define SENVCFG_FIOM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define SATP_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 60)

`define SATP_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 60)

`define SATP_ASID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 44)

`define SATP_ASID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 44)

`define SATP_PPN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 43, 0)

`define SATP_PPN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 43, 0)

`define SRMCFG_MCID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 16)

`define SRMCFG_MCID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 16)

`define SRMCFG_RCID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 0)

`define SRMCFG_RCID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 0)

`define SISELECT_RSVD_63_9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 9)

`define SISELECT_RSVD_63_9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 9)

`define SISELECT_INTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define SISELECT_INTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define SIREG_SIREG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define SIREG_SIREG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define STOPEI_RSVD_63_27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 27)

`define STOPEI_RSVD_63_27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 27)

`define STOPEI_IDENTITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 16)

`define STOPEI_IDENTITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 16)

`define STOPEI_RSVD_15_11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 11)

`define STOPEI_RSVD_15_11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 11)

`define STOPEI_PRIORITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 0)

`define STOPEI_PRIORITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 0)

`define STOPI_RSVD_63_28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 28)

`define STOPI_RSVD_63_28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 28)

`define STOPI_IID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 16)

`define STOPI_IID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 16)

`define STOPI_RSVD_15_8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 8)

`define STOPI_RSVD_15_8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 8)

`define STOPI_IPRIO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define STOPI_IPRIO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define SEED_OPST_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 30)

`define SEED_OPST_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 30)

`define SEED_RSVD_29_24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 29, 24)

`define SEED_RSVD_29_24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 29, 24)

`define SEED_CUSTOM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 16)

`define SEED_CUSTOM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 16)

`define SEED_ENTROPY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 0)

`define SEED_ENTROPY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 0)

`define FFLAGS_NV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define FFLAGS_NV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define FFLAGS_DZ_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define FFLAGS_DZ_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define FFLAGS_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define FFLAGS_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define FFLAGS_UF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define FFLAGS_UF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define FFLAGS_NX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define FFLAGS_NX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define FRM_FRM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 0)

`define FRM_FRM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 0)

`define FCSR_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 8)

`define FCSR_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 8)

`define FCSR_FRM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 5)

`define FCSR_FRM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 5)

`define FCSR_NV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define FCSR_NV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define FCSR_DZ_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define FCSR_DZ_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define FCSR_OF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define FCSR_OF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define FCSR_UF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define FCSR_UF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define FCSR_NX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define FCSR_NX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define VSTART_VSTART_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define VSTART_VSTART_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define VXSAT_VXSAT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define VXSAT_VXSAT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define VXRM_VXRM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 0)

`define VXRM_VXRM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 0)

`define VCSR_VXRM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 1)

`define VCSR_VXRM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 1)

`define VCSR_VXSAT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define VCSR_VXSAT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define VL_VL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VL_VL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define VTYPE_VILL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define VTYPE_VILL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define VTYPE_RESERVED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 8)

`define VTYPE_RESERVED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 8)

`define VTYPE_VMA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define VTYPE_VMA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define VTYPE_VTA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define VTYPE_VTA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define VTYPE_VSEW_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 3)

`define VTYPE_VSEW_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 3)

`define VTYPE_VLMUL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 0)

`define VTYPE_VLMUL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 0)

`define VLENB_VLENB_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VLENB_VLENB_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define PMPCFG0_PMP7CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define PMPCFG0_PMP7CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define PMPCFG0_PMP7CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 61)

`define PMPCFG0_PMP7CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 61)

`define PMPCFG0_PMP7CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 59)

`define PMPCFG0_PMP7CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 59)

`define PMPCFG0_PMP7CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 56)

`define PMPCFG0_PMP7CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 56)

`define PMPCFG0_PMP6CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 55)

`define PMPCFG0_PMP6CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 55)

`define PMPCFG0_PMP6CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 54, 53)

`define PMPCFG0_PMP6CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 54, 53)

`define PMPCFG0_PMP6CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 52, 51)

`define PMPCFG0_PMP6CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 52, 51)

`define PMPCFG0_PMP6CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 50, 48)

`define PMPCFG0_PMP6CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 50, 48)

`define PMPCFG0_PMP5CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 47, 47)

`define PMPCFG0_PMP5CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 47, 47)

`define PMPCFG0_PMP5CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 46, 45)

`define PMPCFG0_PMP5CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 46, 45)

`define PMPCFG0_PMP5CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 44, 43)

`define PMPCFG0_PMP5CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 44, 43)

`define PMPCFG0_PMP5CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 42, 40)

`define PMPCFG0_PMP5CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 42, 40)

`define PMPCFG0_PMP4CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 39, 39)

`define PMPCFG0_PMP4CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 39, 39)

`define PMPCFG0_PMP4CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 38, 37)

`define PMPCFG0_PMP4CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 38, 37)

`define PMPCFG0_PMP4CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 36, 35)

`define PMPCFG0_PMP4CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 36, 35)

`define PMPCFG0_PMP4CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 34, 32)

`define PMPCFG0_PMP4CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 34, 32)

`define PMPCFG0_PMP3CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define PMPCFG0_PMP3CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define PMPCFG0_PMP3CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 29)

`define PMPCFG0_PMP3CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 29)

`define PMPCFG0_PMP3CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 27)

`define PMPCFG0_PMP3CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 27)

`define PMPCFG0_PMP3CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 24)

`define PMPCFG0_PMP3CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 24)

`define PMPCFG0_PMP2CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define PMPCFG0_PMP2CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define PMPCFG0_PMP2CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 21)

`define PMPCFG0_PMP2CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 21)

`define PMPCFG0_PMP2CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 19)

`define PMPCFG0_PMP2CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 19)

`define PMPCFG0_PMP2CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 16)

`define PMPCFG0_PMP2CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 16)

`define PMPCFG0_PMP1CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define PMPCFG0_PMP1CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define PMPCFG0_PMP1CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 13)

`define PMPCFG0_PMP1CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 13)

`define PMPCFG0_PMP1CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define PMPCFG0_PMP1CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define PMPCFG0_PMP1CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 8)

`define PMPCFG0_PMP1CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 8)

`define PMPCFG0_PMP0CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define PMPCFG0_PMP0CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define PMPCFG0_PMP0CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 5)

`define PMPCFG0_PMP0CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 5)

`define PMPCFG0_PMP0CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 3)

`define PMPCFG0_PMP0CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 3)

`define PMPCFG0_PMP0CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 0)

`define PMPCFG0_PMP0CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 0)

`define PMPCFG2_PMP15CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define PMPCFG2_PMP15CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define PMPCFG2_PMP15CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 61)

`define PMPCFG2_PMP15CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 61)

`define PMPCFG2_PMP15CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 59)

`define PMPCFG2_PMP15CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 59)

`define PMPCFG2_PMP15CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 56)

`define PMPCFG2_PMP15CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 56)

`define PMPCFG2_PMP14CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 55)

`define PMPCFG2_PMP14CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 55)

`define PMPCFG2_PMP14CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 54, 53)

`define PMPCFG2_PMP14CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 54, 53)

`define PMPCFG2_PMP14CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 52, 51)

`define PMPCFG2_PMP14CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 52, 51)

`define PMPCFG2_PMP14CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 50, 48)

`define PMPCFG2_PMP14CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 50, 48)

`define PMPCFG2_PMP13CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 47, 47)

`define PMPCFG2_PMP13CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 47, 47)

`define PMPCFG2_PMP13CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 46, 45)

`define PMPCFG2_PMP13CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 46, 45)

`define PMPCFG2_PMP13CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 44, 43)

`define PMPCFG2_PMP13CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 44, 43)

`define PMPCFG2_PMP13CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 42, 40)

`define PMPCFG2_PMP13CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 42, 40)

`define PMPCFG2_PMP12CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 39, 39)

`define PMPCFG2_PMP12CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 39, 39)

`define PMPCFG2_PMP12CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 38, 37)

`define PMPCFG2_PMP12CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 38, 37)

`define PMPCFG2_PMP12CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 36, 35)

`define PMPCFG2_PMP12CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 36, 35)

`define PMPCFG2_PMP12CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 34, 32)

`define PMPCFG2_PMP12CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 34, 32)

`define PMPCFG2_PMP11CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define PMPCFG2_PMP11CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define PMPCFG2_PMP11CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 29)

`define PMPCFG2_PMP11CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 29)

`define PMPCFG2_PMP11CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 27)

`define PMPCFG2_PMP11CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 27)

`define PMPCFG2_PMP11CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 24)

`define PMPCFG2_PMP11CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 24)

`define PMPCFG2_PMP10CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define PMPCFG2_PMP10CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define PMPCFG2_PMP10CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 21)

`define PMPCFG2_PMP10CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 21)

`define PMPCFG2_PMP10CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 19)

`define PMPCFG2_PMP10CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 19)

`define PMPCFG2_PMP10CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 16)

`define PMPCFG2_PMP10CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 16)

`define PMPCFG2_PMP9CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define PMPCFG2_PMP9CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define PMPCFG2_PMP9CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 13)

`define PMPCFG2_PMP9CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 13)

`define PMPCFG2_PMP9CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define PMPCFG2_PMP9CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define PMPCFG2_PMP9CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 8)

`define PMPCFG2_PMP9CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 8)

`define PMPCFG2_PMP8CFG_LOCKED_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define PMPCFG2_PMP8CFG_LOCKED_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define PMPCFG2_PMP8CFG_RSVD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 5)

`define PMPCFG2_PMP8CFG_RSVD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 5)

`define PMPCFG2_PMP8CFG_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 3)

`define PMPCFG2_PMP8CFG_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 3)

`define PMPCFG2_PMP8CFG_RWX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 0)

`define PMPCFG2_PMP8CFG_RWX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 0)

`define PMPADDR0_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR0_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR0_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR0_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR1_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR1_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR1_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR1_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR2_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR2_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR2_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR2_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR3_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR3_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR3_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR3_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR4_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR4_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR4_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR4_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR5_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR5_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR5_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR5_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR6_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR6_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR6_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR6_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR7_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR7_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR7_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR7_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR8_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR8_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR8_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR8_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR9_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR9_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR9_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR9_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR10_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR10_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR10_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR10_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR11_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR11_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR11_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR11_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR12_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR12_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR12_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR12_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR13_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR13_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR13_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR13_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR14_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR14_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR14_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR14_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define PMPADDR15_ADDRESS_HI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 53, 9)

`define PMPADDR15_ADDRESS_HI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 53, 9)

`define PMPADDR15_ADDRESS_LO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define PMPADDR15_ADDRESS_LO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define TSELECT_INDEX_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define TSELECT_INDEX_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define DCSR_XDEBUGVER_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 28)

`define DCSR_XDEBUGVER_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 28)

`define DCSR_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 18)

`define DCSR_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 18)

`define DCSR_EBREAKVS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define DCSR_EBREAKVS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define DCSR_EBREAKVU_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define DCSR_EBREAKVU_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define DCSR_EBREAKM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define DCSR_EBREAKM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define DCSR_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define DCSR_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define DCSR_EBREAKS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define DCSR_EBREAKS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define DCSR_EBREAKU_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define DCSR_EBREAKU_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define DCSR_STEPIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define DCSR_STEPIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define DCSR_STOPCOUNT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define DCSR_STOPCOUNT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define DCSR_STOPTIME_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define DCSR_STOPTIME_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define DCSR_CAUSE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 6)

`define DCSR_CAUSE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 6)

`define DCSR_V_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define DCSR_V_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define DCSR_MPRVEN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define DCSR_MPRVEN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define DCSR_NMIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define DCSR_NMIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define DCSR_STEP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define DCSR_STEP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define DCSR_PRV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 0)

`define DCSR_PRV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 0)

`define DPC_DPC_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define DPC_DPC_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define DSCRATCH0_DSCRATCH0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define DSCRATCH0_DSCRATCH0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define DSCRATCH1_DSCRATCH1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define DSCRATCH1_DSCRATCH1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HSTATUS_WPRI_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 50)

`define HSTATUS_WPRI_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 50)

`define HSTATUS_HUPMM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 49, 48)

`define HSTATUS_HUPMM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 49, 48)

`define HSTATUS_WPRI_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 47, 34)

`define HSTATUS_WPRI_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 47, 34)

`define HSTATUS_VSXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define HSTATUS_VSXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define HSTATUS_WPRI_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 23)

`define HSTATUS_WPRI_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 23)

`define HSTATUS_VTSR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define HSTATUS_VTSR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define HSTATUS_VTW_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define HSTATUS_VTW_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define HSTATUS_VTVM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define HSTATUS_VTVM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define HSTATUS_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 18)

`define HSTATUS_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 18)

`define HSTATUS_VGEIN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 12)

`define HSTATUS_VGEIN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 12)

`define HSTATUS_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 10)

`define HSTATUS_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 10)

`define HSTATUS_HU_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HSTATUS_HU_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HSTATUS_SPVP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define HSTATUS_SPVP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define HSTATUS_SPV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HSTATUS_SPV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HSTATUS_GVA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HSTATUS_GVA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HSTATUS_VSBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HSTATUS_VSBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HSTATUS_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 0)

`define HSTATUS_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 0)

`define HEDELEG_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 24)

`define HEDELEG_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 24)

`define HEDELEG_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 20)

`define HEDELEG_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 20)

`define HEDELEG_HEDELEG_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 16)

`define HEDELEG_HEDELEG_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 16)

`define HEDELEG_HEDELEG_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define HEDELEG_HEDELEG_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define HEDELEG_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define HEDELEG_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define HEDELEG_HEDELEG_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 12)

`define HEDELEG_HEDELEG_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 12)

`define HEDELEG_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 9)

`define HEDELEG_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 9)

`define HEDELEG_HEDELEG_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define HEDELEG_HEDELEG_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define HIDELEG_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define HIDELEG_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define HIDELEG_HARD0_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define HIDELEG_HARD0_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define HIDELEG_VSEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define HIDELEG_VSEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define HIDELEG_HARD0_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HIDELEG_HARD0_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HIDELEG_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HIDELEG_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HIDELEG_VSTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HIDELEG_VSTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HIDELEG_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HIDELEG_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HIDELEG_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define HIDELEG_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define HIDELEG_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HIDELEG_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HIDELEG_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HIDELEG_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HVIP_LCOFIP_VIRT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define HVIP_LCOFIP_VIRT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define HVIP_HARD0_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define HVIP_HARD0_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define HVIP_VSEIP_VIRT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define HVIP_VSEIP_VIRT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define HVIP_HARD0_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HVIP_HARD0_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HVIP_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HVIP_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HVIP_VSTIP_VIRT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HVIP_VSTIP_VIRT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HVIP_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HVIP_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HVIP_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define HVIP_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define HVIP_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HVIP_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HVIP_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HVIP_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HVIPRIO1_HVIPRIO1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HVIPRIO1_HVIPRIO1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HVIPRIO2_HVIPRIO2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HVIPRIO2_HVIPRIO2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HIP_SGEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define HIP_SGEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define HIP_HARD0_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define HIP_HARD0_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define HIP_VSEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define HIP_VSEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define HIP_HARD0_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HIP_HARD0_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HIP_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HIP_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HIP_VSTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HIP_VSTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HIP_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HIP_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HIP_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define HIP_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define HIP_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HIP_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HIP_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HIP_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HIE_SGEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define HIE_SGEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define HIE_HARD0_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define HIE_HARD0_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define HIE_VSEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define HIE_VSEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define HIE_HARD0_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HIE_HARD0_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HIE_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HIE_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HIE_VSTIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HIE_VSTIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HIE_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HIE_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HIE_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define HIE_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define HIE_VSSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HIE_VSSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HIE_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HIE_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HGEIP_GUESTEXTERNALINTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 1)

`define HGEIP_GUESTEXTERNALINTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 1)

`define HGEIP_HARD0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define HGEIP_HARD0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define HGEIE_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 6)

`define HGEIE_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 6)

`define HGEIE_GUESTEXTERNALINTERRUPTSWARL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 1)

`define HGEIE_GUESTEXTERNALINTERRUPTSWARL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 1)

`define HGEIE_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define HGEIE_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define HENVCFG_VSTCE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define HENVCFG_VSTCE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define HENVCFG_PBMTE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define HENVCFG_PBMTE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define HENVCFG_HADE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define HENVCFG_HADE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define HENVCFG_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 34)

`define HENVCFG_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 34)

`define HENVCFG_PMM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define HENVCFG_PMM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define HENVCFG_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 8)

`define HENVCFG_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 8)

`define HENVCFG_CBZE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HENVCFG_CBZE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HENVCFG_CBCFE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HENVCFG_CBCFE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HENVCFG_CBIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 4)

`define HENVCFG_CBIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 4)

`define HENVCFG_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 1)

`define HENVCFG_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 1)

`define HENVCFG_FIOM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define HENVCFG_FIOM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define HCOUNTEREN_HPM31_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 31)

`define HCOUNTEREN_HPM31_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 31)

`define HCOUNTEREN_HPM30_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 30)

`define HCOUNTEREN_HPM30_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 30)

`define HCOUNTEREN_HPM29_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 29, 29)

`define HCOUNTEREN_HPM29_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 29, 29)

`define HCOUNTEREN_HPM28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 28, 28)

`define HCOUNTEREN_HPM28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 28, 28)

`define HCOUNTEREN_HPM27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 27)

`define HCOUNTEREN_HPM27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 27)

`define HCOUNTEREN_HPM26_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 26)

`define HCOUNTEREN_HPM26_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 26)

`define HCOUNTEREN_HPM25_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 25)

`define HCOUNTEREN_HPM25_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 25)

`define HCOUNTEREN_HPM24_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 24, 24)

`define HCOUNTEREN_HPM24_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 24, 24)

`define HCOUNTEREN_HPM23_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 23, 23)

`define HCOUNTEREN_HPM23_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 23, 23)

`define HCOUNTEREN_HPM22_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 22, 22)

`define HCOUNTEREN_HPM22_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 22, 22)

`define HCOUNTEREN_HPM21_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 21)

`define HCOUNTEREN_HPM21_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 21)

`define HCOUNTEREN_HPM20_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 20, 20)

`define HCOUNTEREN_HPM20_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 20, 20)

`define HCOUNTEREN_HPM19_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define HCOUNTEREN_HPM19_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define HCOUNTEREN_HPM18_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define HCOUNTEREN_HPM18_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define HCOUNTEREN_HPM17_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define HCOUNTEREN_HPM17_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define HCOUNTEREN_HPM16_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 16)

`define HCOUNTEREN_HPM16_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 16)

`define HCOUNTEREN_HPM15_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 15)

`define HCOUNTEREN_HPM15_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 15)

`define HCOUNTEREN_HPM14_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 14)

`define HCOUNTEREN_HPM14_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 14)

`define HCOUNTEREN_HPM13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define HCOUNTEREN_HPM13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define HCOUNTEREN_HPM12_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 12)

`define HCOUNTEREN_HPM12_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 12)

`define HCOUNTEREN_HPM11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 11)

`define HCOUNTEREN_HPM11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 11)

`define HCOUNTEREN_HPM10_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 10)

`define HCOUNTEREN_HPM10_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 10)

`define HCOUNTEREN_HPM9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HCOUNTEREN_HPM9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HCOUNTEREN_HPM8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define HCOUNTEREN_HPM8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define HCOUNTEREN_HPM7_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define HCOUNTEREN_HPM7_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define HCOUNTEREN_HPM6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define HCOUNTEREN_HPM6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define HCOUNTEREN_HPM5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define HCOUNTEREN_HPM5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define HCOUNTEREN_HPM4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 4)

`define HCOUNTEREN_HPM4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 4)

`define HCOUNTEREN_HPM3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define HCOUNTEREN_HPM3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define HCOUNTEREN_IR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HCOUNTEREN_IR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HCOUNTEREN_TM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HCOUNTEREN_TM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HCOUNTEREN_CY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define HCOUNTEREN_CY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define HTIMEDELTA_HTIMEDELTA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HTIMEDELTA_HTIMEDELTA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HTVAL_HTVAL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HTVAL_HTVAL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HTINST_HTINST_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define HTINST_HTINST_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define HGATP_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 60)

`define HGATP_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 60)

`define HGATP_WARL0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 58)

`define HGATP_WARL0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 58)

`define HGATP_VMID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 44)

`define HGATP_VMID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 44)

`define HGATP_PPN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 43, 0)

`define HGATP_PPN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 43, 0)

`define HVIEN_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define HVIEN_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define HVIEN_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 0)

`define HVIEN_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 0)

`define HVICTL_VTI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 30, 30)

`define HVICTL_VTI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 30, 30)

`define HVICTL_IID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 21, 16)

`define HVICTL_IID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 21, 16)

`define HVICTL_DPR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define HVICTL_DPR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define HVICTL_IPRIOM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define HVICTL_IPRIOM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define HVICTL_IPRIO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define HVICTL_IPRIO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define VSSTATUS_SD_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define VSSTATUS_SD_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define VSSTATUS_VSSTATUS_WPRI_6_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 34)

`define VSSTATUS_VSSTATUS_WPRI_6_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 34)

`define VSSTATUS_UXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 33, 32)

`define VSSTATUS_UXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 33, 32)

`define VSSTATUS_VSSTATUS_WPRI_5_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 20)

`define VSSTATUS_VSSTATUS_WPRI_5_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 20)

`define VSSTATUS_MXR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 19, 19)

`define VSSTATUS_MXR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 19, 19)

`define VSSTATUS_SUM_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 18, 18)

`define VSSTATUS_SUM_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 18, 18)

`define VSSTATUS_VSSTATUS_WPRI_4_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 17, 17)

`define VSSTATUS_VSSTATUS_WPRI_4_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 17, 17)

`define VSSTATUS_XS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 16, 15)

`define VSSTATUS_XS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 16, 15)

`define VSSTATUS_FS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 14, 13)

`define VSSTATUS_FS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 14, 13)

`define VSSTATUS_VSSTATUS_WPRI_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define VSSTATUS_VSSTATUS_WPRI_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define VSSTATUS_VS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 9)

`define VSSTATUS_VS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 9)

`define VSSTATUS_SPP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 8)

`define VSSTATUS_SPP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 8)

`define VSSTATUS_VSSTATUS_WPRI_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define VSSTATUS_VSSTATUS_WPRI_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define VSSTATUS_UBE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 6, 6)

`define VSSTATUS_UBE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 6, 6)

`define VSSTATUS_SPIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define VSSTATUS_SPIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define VSSTATUS_VSSTATUS_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 4, 2)

`define VSSTATUS_VSSTATUS_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 4, 2)

`define VSSTATUS_SIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define VSSTATUS_SIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define VSSTATUS_VSSTATUS_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define VSSTATUS_VSSTATUS_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define VSIP_LCOFIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define VSIP_LCOFIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define VSIP_HARD0_3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define VSIP_HARD0_3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define VSIP_VSEIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define VSIP_VSEIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define VSIP_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 6)

`define VSIP_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 6)

`define VSIP_VSTIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define VSIP_VSTIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define VSIP_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 2)

`define VSIP_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 2)

`define VSIP_VSSIP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define VSIP_VSSIP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define VSIE_LCOFIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 13, 13)

`define VSIE_LCOFIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 13, 13)

`define VSIE_HARD0_2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 10)

`define VSIE_HARD0_2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 10)

`define VSIE_VSEIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 9, 9)

`define VSIE_VSEIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 9, 9)

`define VSIE_HARD0_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 6)

`define VSIE_HARD0_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 6)

`define VSIE_VSTIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define VSIE_VSTIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define VSIE_HARD0_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 2)

`define VSIE_HARD0_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 2)

`define VSIE_VSSIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define VSIE_VSSIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define VSTVEC_BASESXLEN12WARL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 2)

`define VSTVEC_BASESXLEN12WARL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 2)

`define VSTVEC_MODE_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define VSTVEC_MODE_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define VSTVEC_MODE_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define VSTVEC_MODE_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define VSSCRATCH_SSCRATCH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VSSCRATCH_SSCRATCH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define VSEPC_ADDR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 1)

`define VSEPC_ADDR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 1)

`define VSCAUSE_INTERRUPT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define VSCAUSE_INTERRUPT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define VSCAUSE_EXCEPTIONCODEWLRL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define VSCAUSE_EXCEPTIONCODEWLRL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define VSTVAL_VSTVAL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VSTVAL_VSTVAL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define VSTIMECMP_VSTIMECMP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VSTIMECMP_VSTIMECMP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define VSATP_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 60)

`define VSATP_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 60)

`define VSATP_ASID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 44)

`define VSATP_ASID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 44)

`define VSATP_PPN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 43, 0)

`define VSATP_PPN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 43, 0)

`define VSISELECT_RSVD_63_9_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 9)

`define VSISELECT_RSVD_63_9_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 9)

`define VSISELECT_INTERRUPTS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 8, 0)

`define VSISELECT_INTERRUPTS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 8, 0)

`define VSIREG_SIREG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define VSIREG_SIREG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define VSTOPEI_RSVD_63_27_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 27)

`define VSTOPEI_RSVD_63_27_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 27)

`define VSTOPEI_IDENTITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 26, 16)

`define VSTOPEI_IDENTITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 26, 16)

`define VSTOPEI_RSVD_15_11_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 11)

`define VSTOPEI_RSVD_15_11_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 11)

`define VSTOPEI_PRIORITY_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 10, 0)

`define VSTOPEI_PRIORITY_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 10, 0)

`define VSTOPI_RSVD_63_28_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 28)

`define VSTOPI_RSVD_63_28_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 28)

`define VSTOPI_IID_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 27, 16)

`define VSTOPI_IID_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 27, 16)

`define VSTOPI_RSVD_15_8_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 15, 8)

`define VSTOPI_RSVD_15_8_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 15, 8)

`define VSTOPI_IPRIO_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 0)

`define VSTOPI_IPRIO_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 0)

`define MTINST_MTINST_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MTINST_MTINST_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MTVAL2_MTVAL2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MTVAL2_MTVAL2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define SCOUNTOVF_SCOUNTOVF_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 0)

`define SCOUNTOVF_SCOUNTOVF_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 0)

`define MNSTATUS_MNPP_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 12, 11)

`define MNSTATUS_MNPP_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 12, 11)

`define MNSTATUS_MNPV_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 7, 7)

`define MNSTATUS_MNPV_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 7, 7)

`define MNSTATUS_NMIE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 3)

`define MNSTATUS_NMIE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 3)

`define MNSCRATCH_MNSCRATCH_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define MNSCRATCH_MNSCRATCH_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MNEPC_ADDR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 1)

`define MNEPC_ADDR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 1)

`define MNCAUSE_INTERRUPT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MNCAUSE_INTERRUPT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MNCAUSE_EXCEPTIONCODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define MNCAUSE_EXCEPTIONCODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define MSTATEEN0_SE0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MSTATEEN0_SE0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MSTATEEN0_ENVCFG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define MSTATEEN0_ENVCFG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define MSTATEEN0_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define MSTATEEN0_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define MSTATEEN0_CSRIND_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define MSTATEEN0_CSRIND_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define MSTATEEN0_AIA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define MSTATEEN0_AIA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define MSTATEEN0_IMSIC_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define MSTATEEN0_IMSIC_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define MSTATEEN0_CONTEXT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 57)

`define MSTATEEN0_CONTEXT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 57)

`define MSTATEEN0_P1P13_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 56, 56)

`define MSTATEEN0_P1P13_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 56, 56)

`define MSTATEEN0_SRMCFG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 55, 55)

`define MSTATEEN0_SRMCFG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 55, 55)

`define MSTATEEN0_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 54, 3)

`define MSTATEEN0_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 54, 3)

`define MSTATEEN0_JVT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define MSTATEEN0_JVT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define MSTATEEN0_FCSR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define MSTATEEN0_FCSR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define MSTATEEN0_C_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define MSTATEEN0_C_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define MSTATEEN1_SE1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MSTATEEN1_SE1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MSTATEEN1_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define MSTATEEN1_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define MSTATEEN2_SE2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MSTATEEN2_SE2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MSTATEEN2_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define MSTATEEN2_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define MSTATEEN3_SE3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define MSTATEEN3_SE3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define MSTATEEN3_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define MSTATEEN3_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define HSTATEEN0_SE0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define HSTATEEN0_SE0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define HSTATEEN0_ENVCFG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 62)

`define HSTATEEN0_ENVCFG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 62)

`define HSTATEEN0_WPRI_1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 61, 61)

`define HSTATEEN0_WPRI_1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 61, 61)

`define HSTATEEN0_CSRIND_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 60, 60)

`define HSTATEEN0_CSRIND_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 60, 60)

`define HSTATEEN0_AIA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 59, 59)

`define HSTATEEN0_AIA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 59, 59)

`define HSTATEEN0_IMSIC_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 58, 58)

`define HSTATEEN0_IMSIC_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 58, 58)

`define HSTATEEN0_CONTEXT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 57, 57)

`define HSTATEEN0_CONTEXT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 57, 57)

`define HSTATEEN0_WPRI_0_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 56, 3)

`define HSTATEEN0_WPRI_0_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 56, 3)

`define HSTATEEN0_JVT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define HSTATEEN0_JVT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define HSTATEEN0_FCSR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define HSTATEEN0_FCSR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define HSTATEEN0_C_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define HSTATEEN0_C_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define HSTATEEN1_SE1_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define HSTATEEN1_SE1_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define HSTATEEN1_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define HSTATEEN1_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define HSTATEEN2_SE2_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define HSTATEEN2_SE2_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define HSTATEEN2_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define HSTATEEN2_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define HSTATEEN3_SE3_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 63)

`define HSTATEEN3_SE3_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 63)

`define HSTATEEN3_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 62, 0)

`define HSTATEEN3_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 62, 0)

`define SSTATEEN0_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 3)

`define SSTATEEN0_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 3)

`define SSTATEEN0_JVT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 2, 2)

`define SSTATEEN0_JVT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 2, 2)

`define SSTATEEN0_FCSR_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 1)

`define SSTATEEN0_FCSR_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 1)

`define SSTATEEN0_C_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define SSTATEEN0_C_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define SSTATEEN1_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 0)

`define SSTATEEN1_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 0)

`define SSTATEEN2_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 0)

`define SSTATEEN2_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 0)

`define SSTATEEN3_WPRI_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 0)

`define SSTATEEN3_WPRI_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 0)

// CSR Alias Defines
// cycle is an alias of mcycle
parameter logic [11:0] CYCLE_ALIAS_OF_ADDR = MCYCLE_ADDR;
// instret is an alias of minstret
parameter logic [11:0] INSTRET_ALIAS_OF_ADDR = MINSTRET_ADDR;
// hpmcounter3 is an alias of mhpmcounter3
parameter logic [11:0] HPMCOUNTER3_ALIAS_OF_ADDR = MHPMCOUNTER3_ADDR;
// hpmcounter4 is an alias of mhpmcounter4
parameter logic [11:0] HPMCOUNTER4_ALIAS_OF_ADDR = MHPMCOUNTER4_ADDR;
// hpmcounter5 is an alias of mhpmcounter5
parameter logic [11:0] HPMCOUNTER5_ALIAS_OF_ADDR = MHPMCOUNTER5_ADDR;
// hpmcounter6 is an alias of mhpmcounter6
parameter logic [11:0] HPMCOUNTER6_ALIAS_OF_ADDR = MHPMCOUNTER6_ADDR;
// hpmcounter7 is an alias of mhpmcounter7
parameter logic [11:0] HPMCOUNTER7_ALIAS_OF_ADDR = MHPMCOUNTER7_ADDR;
// hpmcounter8 is an alias of mhpmcounter8
parameter logic [11:0] HPMCOUNTER8_ALIAS_OF_ADDR = MHPMCOUNTER8_ADDR;
// hpmcounter9 is an alias of mhpmcounter9
parameter logic [11:0] HPMCOUNTER9_ALIAS_OF_ADDR = MHPMCOUNTER9_ADDR;
// hpmcounter10 is an alias of mhpmcounter10
parameter logic [11:0] HPMCOUNTER10_ALIAS_OF_ADDR = MHPMCOUNTER10_ADDR;
// hpmcounter11 is an alias of mhpmcounter11
parameter logic [11:0] HPMCOUNTER11_ALIAS_OF_ADDR = MHPMCOUNTER11_ADDR;
// hpmcounter12 is an alias of mhpmcounter12
parameter logic [11:0] HPMCOUNTER12_ALIAS_OF_ADDR = MHPMCOUNTER12_ADDR;
// hpmcounter13 is an alias of mhpmcounter13
parameter logic [11:0] HPMCOUNTER13_ALIAS_OF_ADDR = MHPMCOUNTER13_ADDR;
// hpmcounter14 is an alias of mhpmcounter14
parameter logic [11:0] HPMCOUNTER14_ALIAS_OF_ADDR = MHPMCOUNTER14_ADDR;
// hpmcounter15 is an alias of mhpmcounter15
parameter logic [11:0] HPMCOUNTER15_ALIAS_OF_ADDR = MHPMCOUNTER15_ADDR;
// hpmcounter16 is an alias of mhpmcounter16
parameter logic [11:0] HPMCOUNTER16_ALIAS_OF_ADDR = MHPMCOUNTER16_ADDR;
// hpmcounter17 is an alias of mhpmcounter17
parameter logic [11:0] HPMCOUNTER17_ALIAS_OF_ADDR = MHPMCOUNTER17_ADDR;
// hpmcounter18 is an alias of mhpmcounter18
parameter logic [11:0] HPMCOUNTER18_ALIAS_OF_ADDR = MHPMCOUNTER18_ADDR;
// hpmcounter19 is an alias of mhpmcounter19
parameter logic [11:0] HPMCOUNTER19_ALIAS_OF_ADDR = MHPMCOUNTER19_ADDR;
// hpmcounter20 is an alias of mhpmcounter20
parameter logic [11:0] HPMCOUNTER20_ALIAS_OF_ADDR = MHPMCOUNTER20_ADDR;
// hpmcounter21 is an alias of mhpmcounter21
parameter logic [11:0] HPMCOUNTER21_ALIAS_OF_ADDR = MHPMCOUNTER21_ADDR;
// hpmcounter22 is an alias of mhpmcounter22
parameter logic [11:0] HPMCOUNTER22_ALIAS_OF_ADDR = MHPMCOUNTER22_ADDR;
// hpmcounter23 is an alias of mhpmcounter23
parameter logic [11:0] HPMCOUNTER23_ALIAS_OF_ADDR = MHPMCOUNTER23_ADDR;
// hpmcounter24 is an alias of mhpmcounter24
parameter logic [11:0] HPMCOUNTER24_ALIAS_OF_ADDR = MHPMCOUNTER24_ADDR;
// hpmcounter25 is an alias of mhpmcounter25
parameter logic [11:0] HPMCOUNTER25_ALIAS_OF_ADDR = MHPMCOUNTER25_ADDR;
// hpmcounter26 is an alias of mhpmcounter26
parameter logic [11:0] HPMCOUNTER26_ALIAS_OF_ADDR = MHPMCOUNTER26_ADDR;
// hpmcounter27 is an alias of mhpmcounter27
parameter logic [11:0] HPMCOUNTER27_ALIAS_OF_ADDR = MHPMCOUNTER27_ADDR;
// hpmcounter28 is an alias of mhpmcounter28
parameter logic [11:0] HPMCOUNTER28_ALIAS_OF_ADDR = MHPMCOUNTER28_ADDR;
// hpmcounter29 is an alias of mhpmcounter29
parameter logic [11:0] HPMCOUNTER29_ALIAS_OF_ADDR = MHPMCOUNTER29_ADDR;
// hpmcounter30 is an alias of mhpmcounter30
parameter logic [11:0] HPMCOUNTER30_ALIAS_OF_ADDR = MHPMCOUNTER30_ADDR;
// hpmcounter31 is an alias of mhpmcounter31
parameter logic [11:0] HPMCOUNTER31_ALIAS_OF_ADDR = MHPMCOUNTER31_ADDR;
// sstatus is an alias of mstatus
parameter logic [11:0] SSTATUS_ALIAS_OF_ADDR = MSTATUS_ADDR;
// sip is an alias of mip
parameter logic [11:0] SIP_ALIAS_OF_ADDR = MIP_ADDR;
// fflags is an alias of fcsr
parameter logic [11:0] FFLAGS_ALIAS_OF_ADDR = FCSR_ADDR;
// frm is an alias of fcsr
parameter logic [11:0] FRM_ALIAS_OF_ADDR = FCSR_ADDR;
// vxsat is an alias of vcsr
parameter logic [11:0] VXSAT_ALIAS_OF_ADDR = VCSR_ADDR;
// vxrm is an alias of vcsr
parameter logic [11:0] VXRM_ALIAS_OF_ADDR = VCSR_ADDR;
// hvip is an alias of mip
parameter logic [11:0] HVIP_ALIAS_OF_ADDR = MIP_ADDR;
// hip is an alias of mip
parameter logic [11:0] HIP_ALIAS_OF_ADDR = MIP_ADDR;
// hie is an alias of mie
parameter logic [11:0] HIE_ALIAS_OF_ADDR = MIE_ADDR;
// vsip is an alias of mip
parameter logic [11:0] VSIP_ALIAS_OF_ADDR = MIP_ADDR;

endpackage : csr_defines_pkg
