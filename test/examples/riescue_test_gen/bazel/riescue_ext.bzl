"""Fetches riescue from GitHub and the RISC-V bare-metal GCC it compiles
with; both get BUILD overlays from this package."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

_RIESCUE_COMMIT = "c78c55ed0e879ce3b4b297c153e02731ba70c46a"
_RIESCUE_SHA256 = "b9d0b7bab48088d6040e46c00e1522a557786bed52b1ec11e4b797a08ae17b93"

# xPack GCC: relocatable, runs on any glibc >= 2.28 host (cvm container
# included — the container has no RISC-V toolchain).
_XPACK_GCC_VERSION = "14.2.0-3"
_XPACK_GCC_SHA256 = "f574415b63f12b09bdd3475223ab492a465d23810646c90c13a4c3b676c83503"

def _ext_impl(_ctx):
    http_archive(
        name = "riescue",
        urls = ["https://github.com/tenstorrent/riescue/archive/{}.tar.gz".format(_RIESCUE_COMMIT)],
        strip_prefix = "riescue-" + _RIESCUE_COMMIT,
        sha256 = _RIESCUE_SHA256,
        build_file = "//test/examples/riescue_test_gen/bazel:riescue.BUILD",
        patches = ["//test/examples/riescue_test_gen/bazel:riescue_eot_sd_tohost.patch"],
        patch_args = ["-p1"],
    )
    http_archive(
        name = "riscv_none_elf_gcc",
        urls = ["https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v{v}/xpack-riscv-none-elf-gcc-{v}-linux-x64.tar.gz".format(v = _XPACK_GCC_VERSION)],
        strip_prefix = "xpack-riscv-none-elf-gcc-" + _XPACK_GCC_VERSION,
        sha256 = _XPACK_GCC_SHA256,
        build_file = "//test/examples/riescue_test_gen/bazel:riscv_none_elf_gcc.BUILD",
    )

riescue_ext = module_extension(implementation = _ext_impl)
