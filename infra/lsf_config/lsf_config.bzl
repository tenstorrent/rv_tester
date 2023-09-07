LSF_OPTS = {
    "global": {
        "lsf_opts": ["-app", "fedocker"], # TODO pass this to bazel-remote
    },
    "vcs": {
        "run": {
            "lsf_opts": ["-R", "rusage[VCSRuntime_Net=1]"],
        }
    }
}
