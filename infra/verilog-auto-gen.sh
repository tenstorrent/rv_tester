#! /usr/bin/env bash

set -e
set -o pipefail

src=$1
out=$2
copy=$(mktemp -p .)
el=$(mktemp)

trap "rm -f $copy $el" EXIT
cp $src $copy

cat > $el << EOF
(load-library "verilog-mode")
(setq find-file-visit-truename t)
(setq verilog-library-flags (list "+incdir+rtl/gen_files" "+incdir+rtl/fe" "+incdir+rtl/mc" "+incdir+rtl/ls" "+incdir+rtl/ci" "+incdir+rtl/ch" "+incdir+rtl/core"))
(verilog-batch-auto)
EOF

emacs --batch -Q $copy -l $el

mv -f $copy $out
echo "Wrote $out"
