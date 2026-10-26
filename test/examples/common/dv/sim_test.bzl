"""Run a test ELF on a core testbench under rv_tester lockstep:
rv_tester_sim_test declares a sh_test for an ELF known to Bazel;
rv_tester_sim_run declares a `bazel run` target taking the ELF (and extra
+plusargs) on the command line."""

_DEFAULT_MEMMAP = "//test/examples/common/dv:memmap.json"
_DEFAULT_WHISPER = "//test/examples/common/dv:whisper.json"

# Public so callers can compose, e.g. DEFAULT_PLUSARGS + C910_PLUSARGS.
DEFAULT_PLUSARGS = [
    "+nomcm",
    "+eot=tohost",
]

def rv_tester_sim_test(
        name,
        tb,
        elf,
        whisper_json = _DEFAULT_WHISPER,
        memmap_json = _DEFAULT_MEMMAP,
        plusargs = DEFAULT_PLUSARGS,
        **kwargs):
    """sh_test running `elf` on the Verilator testbench `tb` in lockstep.

    Args:
      name: test name.
      tb: label of the core's *_tb_verilator cc_binary.
      elf: label of the test ELF.
      whisper_json: cosim whisper config; the //<core>/dv variants correct
        misa/mstatus to mirror the DUT.
      memmap_json: platform memory map.
      plusargs: run-control plusargs; replaces the default list.
      **kwargs: forwarded to sh_test (timeout, tags, ...).
    """
    native.sh_test(
        name = name,
        srcs = ["//test/examples/common/dv:sim.sh"],
        args = [
            "$(location {})".format(tb),
            "+load=$(location {})".format(elf),
            "+memmap_json_path=$(location {})".format(memmap_json),
            "+whisper_json_path=$(location {})".format(whisper_json),
        ] + plusargs,
        data = [
            tb,
            elf,
            memmap_json,
            whisper_json,
        ],
        **kwargs
    )

def rv_tester_sim_run(
        name,
        tb,
        whisper_json = _DEFAULT_WHISPER,
        memmap_json = _DEFAULT_MEMMAP,
        plusargs = DEFAULT_PLUSARGS,
        **kwargs):
    """`bazel run` target: bazel run <name> -- <path/to.elf>|+load=<path/to.elf> [+plusarg...]

    The ELF path is resolved against the invocation directory; simulator
    scratch files land there too unless +run_path=<dir> is given. Same knobs
    as rv_tester_sim_test minus elf.
    """
    native.sh_binary(
        name = name,
        srcs = ["//test/examples/common/dv:sim_run.sh"],
        args = [
            "$(location //test/examples/common/dv:sim.sh)",
            "$(location {})".format(tb),
            "$(location {})".format(memmap_json),
            "$(location {})".format(whisper_json),
        ] + plusargs,
        data = [
            tb,
            memmap_json,
            whisper_json,
            "//test/examples/common/dv:sim.sh",
        ],
        **kwargs
    )
