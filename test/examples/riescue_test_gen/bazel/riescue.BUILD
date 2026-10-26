# BUILD overlay for riescue (ships no Bazel files). The runnable riescued
# entry point lives in //riescue_test_gen: pypi deps are not resolvable from
# this repo's mapping.

load("@rules_python//python:defs.bzl", "py_library")

py_library(
    name = "riescue_lib",
    srcs = glob(["riescue/**/*.py"]),
    # Runtime package data: configs/tables (.json), rvmodel macros (.h),
    # linker fragments (.ld), bundled tests (.s).
    data = glob(
        [
            "riescue/**/*.json",
            "riescue/**/*.h",
            "riescue/**/*.ld",
            "riescue/**/*.s",
        ],
        allow_empty = True,
    ),
    imports = ["."],
    visibility = ["//visibility:public"],
)
