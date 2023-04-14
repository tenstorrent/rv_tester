#! /usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/container-id.sh

groups=""
for group in $(id -G); do
    groups="$groups --group-add=$group"
done

ARGS="$@"
if [[ -z "${ARGS}" ]]; then
    ARGS="bash"
fi

exec docker run --security-opt seccomp=unconfined --rm -it -e http_proxy -e https_proxy -e USER=$(whoami) --user $(id -u):_replaceme $groups -v /proj/ttrentz1:/tools_risc -v /et3mach:/et3mach -v /apps:/apps:shared -v /scratch:/scratch -v /proj:/proj -v /projects:/projects -v /global:/global -v $HOME/.ssh:/home/_replaceme/.ssh -v $HOME/.ssh:$HOME/.ssh -v $HOME/.gitconfig:/home/_replaceme/.gitconfig -v /var/run/nscd/socket:/var/run/nscd/socket -e HOME=/home/_replaceme --net host $CONTAINER_ID bash -c "cd -- $(pwd) && exec $ARGS"
