export SNPSLMD_LICENSE_FILE=${SNPSLMD_LICENSE_FILE:-27020@yyz-license-02:27020@yyz-license-01:27020@yyz-colo-lsf-test}
export LM_LICENSE_FILE=${LM_LICENSE_FILE:-5280@aus-license-1}
export VCS_HOME=${VCS_HOME:-/home/soc_tools/synopsys/vcs/T-2022.06}
export VERDI_HOME=${VERDI_HOME:-/home/soc_tools/synopsys/verdi/T-2022.06}
export SPYGLASS_HOME=${SPYGLASS_HOME:-/home/soc_tools/synopsys/spyglass/S-2021.09-1/SPYGLASS_HOME}
export VC_STATIC_HOME=${VC_STATIC_HOME:-/tools_vendor/synopsys/vc_static/T-2022.06-SP2}
export CDN_JG_HOME=${CD_JG_HOME:-/tools_vendor/cadence/jasper/2022.12}
export VXE_HOME=${VXE_HOME:-/tools_vendor/cadence/vxe/VXE2204}
export IXCOM_HOME=${IXCOM_HOME:-/tools_vendor/cadence/ixcom/IXCOM2204}
export HDLICE_HOME=${HDLICE_HOME:-/tools_vendor/cadence/hdlice/HDLICE2204}
export XCELIUM_HOME=${XCELIUM_HOME:-/tools_vendor/cadence/xcelium/XCELIUM2203}
export PATH=/opt/rh/gcc-toolset-11/root/usr/bin:${PATH}:${VCS_HOME}/bin:${VERDI_HOME}/bin:${SPYGLASS_HOME}/bin:${VC_STATIC_HOME}/bin:${CDN_JG_HOME}/bin:${XCELIUM_HOME}/tools.lnx86/inca/bin/64bit:$XCELIUM_HOME/bin
export CC=${CC:-$(which gcc)}
export SNPS_SIM_DEFAULT_GUI=verdi
export VERDI_ENHANCE_DYNAMIC_OBJECT=1
export NOVAS_IDLE_LICENSE_CHECKBACK=60
export BZSIM_RABBIT_SERVER="10.220.20.68 10.220.20.69"
export BZSIM_RABBIT_EXCHANGE=riscv
export BZSIM_SITE=aus
export BZSIM_RABBIT_ROUTING_KEY=ascalon.bzsim.bep.
export RISC_P_CORES_ENV_SOURCED=1
export RESPOSITORY_NAME=risc-p-cores
export COSMOS_LIB_PATH=/tools_risc/tt/cosmos/v1.1.9/backend/lib

# Enable --lsf and --bazel-remote-dir only if austin servers
if [[ $(uname -n) == *"aus"* ]]; then 
    export BZSIM_LSF=1
    export BZSIM_BAZEL_REMOTE_DIR=/weka_scratch/rv_bazel_cache/v3
fi 
