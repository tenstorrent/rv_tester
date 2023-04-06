#! /usr/bin/env python3

import argparse
import os
import pathlib
import subprocess
import getpass
from datetime import datetime

class Git(object):

    @staticmethod
    def run(*args, **kwargs):
        return subprocess.run(["git"] + list(args), check = True, **kwargs)

    @staticmethod
    def get(*args):
        return Git.run(*args, stdout = subprocess.PIPE, universal_newlines = True).stdout.rstrip()

    @staticmethod
    def commit_sha(ref):
        return Git.get("rev-parse", ref)

    @staticmethod
    def branch_name(ref = "HEAD"):
        return Git.get("rev-parse", "--abbrev-ref", ref)

    @staticmethod
    def commit_title():
        return Git.get("log", "--pretty=%s", "-1")

    @staticmethod
    def rev_list(start, end, n = None):
        a = ["rev-list", Git.commit_sha(end), "^" + Git.commit_sha(start)]
        if n is not None:
            a += ["-n", str(n)]
        return Git.get(*a)

    @staticmethod
    def reset(ref):
        Git.run("reset", "--hard", ref)


class Checkin(object):

    def yes_or_no(self, default=None):

        answer = input().lower()

        if default is not None and not answer:
            return default
        elif answer == 'y':
            return True
        elif answer == 'n':
            return False

        raise ValueError()

    def ask(self, question, default=None):

        prompt = {
            True : "[Y/n]",
            False: "[y/N]",
            None : "[y/n]",
        }[default]

        print(question + "\n" + prompt)

        while True:
            try:
                return self.yes_or_no(default)
            except ValueError:
                print("please respond with y or n")

    def __init__(self):

        script_path = pathlib.Path(os.path.abspath(__file__))
        with open(script_path.parent / "chip", "r") as f:
            chip = f.read().rstrip()

        user = getpass.getuser()

        description = f"""
        Convenience tool to push a merge branch.
        Merge branches will have CI autorun smoke on them and merge into the main branch if CI passes.
        """
        parser = argparse.ArgumentParser(description=description)
        parser.add_argument("--remote", default="origin")
        parser.add_argument("--head"  , default="HEAD"  )
        parser.add_argument("--target", default=chip    )
        parser.add_argument("--branch")
        parser.add_argument("--force-new-branch", default=False, action="store_true")
        parser.add_argument("--user"            , default=user)
        parser.add_argument("--assignee"        )
        parser.add_argument("--no-mr"           , default=False, action="store_true")
        parser.add_argument("--reset"           , default=False, action="store_true")
        parser.add_argument("--skip_commit_check", default=False, action="store_true")

        args = parser.parse_args()

        branch = args.branch
        no_mr  = args.no_mr

        if not branch:
            head_name = Git.branch_name(args.head)
            if head_name.startswith(f"merge/{chip}"):
                if not args.force_new_branch:
                    existing = self.ask(f"You're already on a merge branch. Do you want to push to the exisiting branch {head_name} instead of creating a new one?", True)
                    if existing:
                        branch = head_name
                        no_mr  = True

        if not branch:
            date = datetime.now().strftime('%Y%m%d%H%M%S')
            unique_name = f"{args.user}-{date}"
            branch = f"merge/{chip}/{unique_name}"

        self.remote     = args.remote
        self.head       = args.head
        self.branch     = branch
        self.target     = args.target
        self.assignee   = args.assignee or args.user
        self.no_mr      = no_mr
        self.reset      = args.reset
        self.skip_commit_check = args.skip_commit_check

    def check_pushing_new_commit(self):

        if not Git.rev_list(f"{self.remote}/{self.target}", self.head, 1):
            draft = self.ask(f"{self.remote}/{self.target} already contains all of the commits in {self.head}. Perhaps you forget to do `git commit`. Do you want to continue pushing a branch and create a merge request anyways?", False)
            return draft

        return True

    def push(self):

        if not self.skip_commit_check and not self.check_pushing_new_commit():
            print("Nothing to push, exiting")
            return

        mr = []
        if not self.no_mr:
            mr = [
                "-o", "merge_request.create",
                "-o", "merge_request.target=" + self.target,
                "-o", "merge_request.merge_when_pipeline_succeeds",
                "-o", "merge_request.remove_source_branch",
                "-o", "merge_request.title=Checkin script:" + Git.commit_title()[0:50],
                "-o", 'merge_request.label=checkin-script',
                "-o", "merge_request.assign=" + self.assignee,
            ]

        Git.run(*([
            "push", self.remote, f"{self.head}:refs/heads/{self.branch}",
        ] + mr))

        if self.reset:
            Git.reset(f"{self.remote}/{self.target}")



if __name__ == "__main__":

    Checkin().push()

