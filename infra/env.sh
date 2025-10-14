export SNPSLMD_LICENSE_FILE="${SNPSLMD_LICENSE_FILE:-27020@l1t5.csl.cloud.synopsys.com:27020@aus-license-1:27020@o3v0.csl.cloud.synopsys.com:27020@yyz-license-02:27020@yyz-license-01:27020@m8e2.csl.cloud.synopsys.com:27020@g9z2.csl.cloud.synopsys.com}"
export LM_LICENSE_FILE="${LM_LICENSE_FILE:-5280@aus-license-1:2100@aus-license-1}"
export VCS_HOME=${VCS_HOME:-/tools_vendor/synopsys/vcs/T-2022.06}
export VERDI_HOME=${VERDI_HOME:-/tools_vendor/synopsys/verdi/T-2022.06}
export SPYGLASS_HOME=${SPYGLASS_HOME:-/tools_vendor/synopsys/spyglass/S-2021.09-1/SPYGLASS_HOME}
export VC_STATIC_HOME=${VC_STATIC_HOME:-/tools_vendor/synopsys/vc_static/T-2022.06-SP2}
export CDN_JG_HOME=${CD_JG_HOME:-/tools_vendor/cadence/jasper/2022.12}
export VXE_HOME=${VXE_HOME:-/tools_vendor/cadence/vxe/VXE2204}
export IXCOM_HOME=${IXCOM_HOME:-/tools_vendor/cadence/ixcom/IXCOM2204}
export HDLICE_HOME=${HDLICE_HOME:-/tools_vendor/cadence/hdlice/HDLICE2204}
export XCELIUM_HOME=${XCELIUM_HOME:-/tools_vendor/cadence/xcelium/XCELIUM2203}

if [[ $(uname -n) == "sc39pd01"  ]]; then
    export XCELIUM_HOME=/apps/XCELIUM2203/22.03.007
    export CDS_LIC_FILE=5280@10.111.15.129 #5280@sc39ut01
    export LM_LICENSE_FILE=5280@10.111.15.129 #5280@sc39ut01
    export VXE_HOME=/apps/VXE2204/22.04.001
    export IXCOM_HOME=/proj/ttrentz1/mchandna/IXCOM_fix_bld_0306
    export HDLICE_HOME=/apps/HDLICE_22.04.160-s002_230220
    export LD_LIBRARY_PATH=${VXE_HOME}/share/vxe/gift/fsdbWriter_57:$LD_LIBRARY_PATH
fi

export PATH=${PATH}:${VCS_HOME}/bin:${VERDI_HOME}/bin:${SPYGLASS_HOME}/bin:${VC_STATIC_HOME}/bin:${CDN_JG_HOME}/bin:${XCELIUM_HOME}/tools.lnx86/inca/bin/64bit:$XCELIUM_HOME/bin:${VXE_HOME}/bin:${IXCOM_HOME}/bin:${HDLICE_HOME}/bin:${LDVHOME}/tools/bin:$XCELIUM_HOME/bin:$XCELIUM_HOME/tools/bin
export CC=${CC:-$(which clang)}
export CXX=${CXX:-$(which clang++)}
export SNPS_SIM_DEFAULT_GUI=verdi
export VERDI_ENHANCE_DYNAMIC_OBJECT=1
export NOVAS_IDLE_LICENSE_CHECKBACK=60
export BZSIM_RABBIT_SERVER="10.220.20.68 10.220.20.69"
export BZSIM_RABBIT_EXCHANGE=riscv
export BZSIM_SITE=aus
export BZSIM_RABBIT_ROUTING_KEY=ascalon.bzsim.bep.
export RISC_P_CORES_ENV_SOURCED=1
export REPOSITORY_NAME=rv_tester
export COSMOS_LIB_PATH=/tools_risc/tt/cosmos/v1.3.0/backend/lib
export SS_ID_SERVER=http://aus-ss:3000

# Enable --lsf and --bazel-remote-dir only if austin servers
if [[ -d /weka_scratch/rv_bazel_cache/v3 ]]; then
    export BZSIM_LSF=1
    export BZSIM_BAZEL_REMOTE_DIR=/weka_scratch/rv_bazel_cache/v3
fi
