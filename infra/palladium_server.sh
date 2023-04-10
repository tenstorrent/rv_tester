#! /usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export LD_LIBRARY_PATH=/proj/ttrentz1/tt/lib:$LD_LIBRARY_PATH
export XE_SERVER_PORT=1234

dir=$(mktemp -d -p . "xeDebugServer.$(date +%Y%m%d%H%M%S).XXXXXX")
cd $dir

echo "6" > top.bp

exec ${SCRIPT_DIR}/env-wrap xeDebug -init ${SCRIPT_DIR}/palladium_server.tcl --xmsim
