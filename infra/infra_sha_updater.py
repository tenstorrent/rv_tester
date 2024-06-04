#! /usr/bin/env python3

import os
import sys
import json
import subprocess

repo = sys.argv[1]
commit = sys.argv[2]

ROOTDIR = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], universal_newlines = True).rstrip()

sha_json_path = os.path.realpath(ROOTDIR) + "/infra/bzsim-config/sha.json"
json_object = None

with open(sha_json_path, 'r') as sha_file:
    sha_json = json.load(sha_file)
    if repo not in sha_json['repos'].keys():
        raise Exception(f'{repo} not found in {sha_file.name}')
    sha_json['repos'][repo]['sha'] = commit
    json_object = json.dumps(sha_json, indent=4)

with open(sha_json_path, 'w') as sha_file:
    sha_file.write(json_object)

