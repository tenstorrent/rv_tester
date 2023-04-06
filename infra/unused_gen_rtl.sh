#! /usr/bin/env bash

set -e
set -o pipefail


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_DIR=$(dirname $SCRIPT_DIR)

(cd ${SCRIPT_DIR} && ${SCRIPT_DIR}/gen_rtl_top.py)
echo $PWD

declare -A verilog_auto_files=(
    ["rtl/core/as_top.sv"]="gen/rtl/core/as_main.sv"
    ["rtl/ls/ls_top.sv"]="gen/rtl/ls/ls_main.sv"
)

for file in ${!verilog_auto_files[@]}; do
    src=${file}
    out=${verilog_auto_files[${file}]}
    if [ -z "$out" ]; then
        out=$src # in-place
    fi
    (cd ${REPO_DIR} && ${SCRIPT_DIR}/verilog-auto-gen.sh ${src} ${out})
done

# echo "Post processing Auto Verilog file to fix the 2D issue"
# cd ${REPO_DIR}/rtl/core
# grep "Core_pkg.*\[" as_top.sv | awk '{print "logic.*"$3}' > temp
# grep -f temp -v as_main_pre.sv > as_main_pre2.sv
# grep "Core_pkg.*\[" as_top.sv | awk '{print $1"\\s*("}' > temp
# grep -f temp -v as_main_pre2.sv > as_main.sv

