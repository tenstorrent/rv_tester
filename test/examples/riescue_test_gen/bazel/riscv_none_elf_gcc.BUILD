# BUILD overlay for the xPack RISC-V GCC tarball. The driver finds as/ld/
# libexec relative to its own realpath, so actions must stage :all_files
# alongside the entry points.

filegroup(
    name = "gcc",
    srcs = ["bin/riscv-none-elf-gcc"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "objdump",
    srcs = ["bin/riscv-none-elf-objdump"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "all_files",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
