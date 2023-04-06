#! /usr/bin/env python3

import argparse
import sys
import os
import subprocess
from pathlib import Path
import json

i = sys.argv.index('--')
remainder = sys.argv[i+1:]
argv = sys.argv[1:i]

parser = argparse.ArgumentParser()
parser.add_argument("--bazel-output-base")
parser.add_argument("--container-run-opts")
parser.add_argument("--container-image")
args = parser.parse_args(argv)

script_dir_path     = Path(os.path.abspath(__file__)).parent
repo_path           = script_dir_path.parent

lsf_profile=Path("/site/lsf/aus-hw/conf/profile.lsf")
if lsf_profile.exists():
    os.environ = json.loads(
        subprocess.check_output(
            f"source {lsf_profile} >& /dev/null &&" + \
            f"{sys.executable} -c 'import os, json;print(json.dumps(dict(os.environ)))'",
            shell=True,
            universal_newlines=True
        )
    )

container_opts = f"--rm -v /tools_vendor:/home/soc_tools -v /tech/tsmc/N06:/home/il_home/tsmc -v {repo_path}:{repo_path} -v /tools_risc/tt/cosmos:/tools_risc/tt/cosmos -v /proj_risc:/proj_risc -v /weka_scratch/rv_bazel_cache:/weka_scratch/rv_bazel_cache -v /tools_risc/tt/cosmos:/tools_risc/tt/cosmos --net host "

if args.bazel_output_base:
    container_opts += f"-v {args.bazel_output_base}:{args.bazel_output_base}"

if args.container_run_opts:
    container_opts += " " + args.container_run_opts

if args.container_image:
    os.environ['LSB_CONTAINER_IMAGE'] = args.container_image

os.environ['LSB_CONTAINER_ADDITIONAL_OPTIONS'] = container_opts
os.environ['LSB_CONTAINER_ADD_USER_GROUPS']='Y'
os.environ['LSF_DOCKER_MOUNT_TMPDIR']='N'
os.environ['DEBUG_CONTAINER_JOB']='Y'

os.execvpe(remainder[0], remainder, os.environ)
