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
parser.add_argument("--container-image")
parser.add_argument("--container-run-opts",
    action = "append",
    default = [],
)
args = parser.parse_args(argv)

script_dir_path     = Path(os.path.abspath(__file__)).parent
repo_path           = script_dir_path.parent
container_run       = script_dir_path / 'container' / 'container_run'

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

container_opts = []
if args.bazel_output_base:
    container_opts.extend(['--opts', f'--bind={args.bazel_output_base}'])

if len(args.container_run_opts):
    container_opts.extend(args.container_run_opts)

if container_run.is_file():
    os.execvpe(str(container_run), [str(container_run), *container_opts] + remainder, os.environ)
else:
    os.execvpe(remainder[0], remainder, os.environ)
