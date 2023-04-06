#! /usr/bin/env python3

from pathlib import Path
import argparse 
import sys 
import os


"""
    Runs rv-common scripts. 

    Defaults to bzsim run @rv-common//infra + script. 
    unit map for more specific targets
"""


class RV_Common_Exception(Exception): pass

class RV_Common:
    alt_map = {}

    def __init__(self, script, args=sys.argv):
        self.script = self.get_script_target(script)
        self.bazel_wrap = Path(__file__).parent / "bazel-wrap"
        self.args = args

        if not self.bazel_wrap.exists():
            raise RV_Common_Exception(f"Couldn't find bazel-wrap at {str(self.bazel_wrap)}")
    
    def run(self):
        import subprocess
        build_cmd =  [self.bazel_wrap, 'build', self.script ] #+ self.args
        print("building "+self.script)
        subprocess.run(build_cmd, check=True, cwd=Path.cwd())
        print(Path.cwd())
        
        query = [self.bazel_wrap, 'cquery', self.script, "--output=files"]
        r = subprocess.run(query, check=True, cwd=Path.cwd(), text=True, stdout=subprocess.PIPE)
        out_files = r.stdout.split()

        target_script = [out_files[-1]] + self.args
        print("running: ", " ".join(target_script))
        subprocess.run(target_script, check=True, cwd=Path.cwd())


    def get_script_target(self, script):
        if script in self.alt_map.keys():
            return self.alt_map(script)
        return "@rv-common//infra:" + script

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("No target passed")
        sys.exit(1)

    script = sys.argv[1]
    args = sys.argv[2:]
    RV_Common(script, args).run()
