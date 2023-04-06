#! /usr/bin/env python3

import os 
import sys
import subprocess
import json
from pathlib import Path


class GitGetterException(Exception):
    pass


'''
    Class used to install git repos as subdirectories.
    Uses sha.json which should be stored in bzsim-config, a directory inside this file's
    parent directory.  

    use "skip=True" in get_git_repo() to skip the sha check, allowing other branches to override 
    you're testing changes in respective repos
'''


class GitGetter:
    def __init__(self, filepath: Path):
        self.filepath = filepath
        self.bzsim_dir = filepath.parent    # e.g filepath = Path(__file__).resolve()   
        self.config_dir = self.bzsim_dir / "bzsim-config"
        if not self.config_dir.exists():
            raise GitGetterException(f"No bzsim-config directory found in bzsim directory: {str(self.bzsim_dir)}")
            
        get_sha_file = self.config_dir / "sha.json"
        self.sha_dict = self.get_sha_dict(get_sha_file)
        try:
            self.skip_sha =self.sha_dict['skip_sha']
        except KeyError:
            self.skip_sha = False
        self.git_repos = self.sha_dict['repos']


    
    # Given path to a sha.json file, returns the dict
    def get_sha_dict(self, sha: Path) -> dict:
        if not sha.exists():
            raise GitGetterException(f"SHA json file not found, ensure that there is a sha.json file is in the directory {str(self.config_dir)}")
        with open(sha, "r") as sha_rdr:
            return json.load(sha_rdr)

    def git_run(self, git_cmd, *args, **kwargs):
        ret = subprocess.run(git_cmd+[*args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8', **kwargs)
        if ret.returncode != 0:
            raise GitGetterException(f"Git command failed:{(' ').join(git_cmd)}\nstdout:{ret.stdout}\nstderr:{ret.stderr}")  
        return ret  

    # Clones repo given the git_url into the script diretory
    def git_clone_repo(self, git_url, repo_dir, sha=None, branch="", *args, **kwargs):
        git_cmd = ["git", "clone"]
        if branch:
            git_cmd.extend(["-b", branch, "--single-branch"])

        git_cmd.extend([git_url, str(repo_dir)])
        self.git_run(git_cmd, *args, **kwargs)

        if sha:
            git_cmd = ["git", "-C", repo_dir , "checkout", sha]
            self.git_run(git_cmd, *args, **kwargs)

    def git_shallow_clone(self, repo_dir: Path, repo_url: str, commit_sha: str, **kwargs):
        repo_dir.mkdir(exist_ok=True)
        self.git_run(["git", "init"], cwd=repo_dir)
        self.git_run(["git", "remote", "add", "origin", repo_url], cwd=repo_dir)
        self.git_run(["git", "fetch", "origin", "--depth=1", commit_sha], cwd=repo_dir)
        self.git_run(["git", "reset", "--hard", "FETCH_HEAD"], cwd=repo_dir)

    def git_pull_repo(self, repo_dir, *args, **kwargs):
        git_cmd = ["git", "-C", str(repo_dir), "pull"]
        self.git_run(git_cmd, *args, **kwargs)

    def get_current_sha(self, repo_dir):
        git_cmd = ["git", "-C", str(repo_dir), "rev-parse", "HEAD"]
        return self.git_run(git_cmd).stdout.strip()

    def git_fetch_and_checkout(self, repo_dir, sha, *args, **kwargs):
        git = ["git", "-C", str(repo_dir)]
        self.git_run(git + ["fetch", "origin", "--depth=1", sha])
        self.git_run(git + ["checkout", sha])

    # Could do git ls-remote but that adds delay due to querying the git server
    def check_ls_remote(self, git_url: str) -> bool:
        git_cmd = ["git", "ls-remote", git_url]
        response = subprocess.run(git_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) 
        if response.returncode != 0:
            return False
        return True

    def get_git_repo(self, git_url, sha, dir_name=None, repo_name=None, skip_sha=False):
        if not ("@" in git_url and ".git" in git_url):
            raise GitGetterException("\n\nInvalid SSH git URL. \nGit URls require an '@' symbol and '.git' in the URL.")
        repo_name = repo_name or dir_name or git_url.split("/")[-1].rstrip(".git") 
        repo_dir = self.bzsim_dir / repo_name
    
        if not repo_dir.exists():
            print("Cloning " + repo_name + " ... ", end="")
            self.git_shallow_clone(repo_dir, git_url, sha)
            print("Done")
        else:
            if not (repo_dir / ".git").exists():
                raise GitGetterException(f"No .git file found in {str(repo_dir)}.\nCannot clone into a non-empty directory, try:\nrm -rf {str(repo_dir)}")
            else:
                print("Checking existing repo " + repo_name + " ... ", end="")
                if sha != self.get_current_sha(repo_dir):
                    if skip_sha:
                        print(f"desired SHA: {sha}", end="\t")
                        print(f"current SHA: {self.get_current_sha(repo_dir)}" )
                        print("skip enabled, not pulling from sha.json's sha ... ",  end="")
                    else:
                        print("fetching " + sha + " ... ", end = "")
                        self.git_fetch_and_checkout(repo_dir, sha)
                    print("Done")
                else:
                    print("Up to date")

    def get_repos(self, skip=False):
        skip_sha = skip or self.skip_sha
        for repo_name in self.git_repos:
            repo = self.git_repos[repo_name]
            self.get_git_repo(repo['url'], repo['sha'], repo_name=repo_name, skip_sha=skip_sha)

if __name__ == "__main__":
    script_path = Path(__file__).resolve()
    infra_dir = script_path.parent

    # Get git repos, import container
    getter = GitGetter(script_path)
    getter.get_repos()
    print("Done")


