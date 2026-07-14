# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

from abc import ABC, abstractmethod
import argparse
import os
import subprocess
from typing import Optional

class Command:
    def __init__(self, arg_list: "list[str]", output_file_name: "Optional[str]" = None):
        self.arg_list = arg_list
        self.output_file_name = output_file_name

    def __str__(self):
        return str(self.arg_list)

class Profiler(ABC):
    def __init__(self, rv_tester_path: str, args: argparse.Namespace):
        self.args = args
        self.rv_tester_path = rv_tester_path
        self.bazel_wrap_cmd = [os.path.join(self.rv_tester_path, "infra", "bazel-wrap")]
        self.bzsim_run_cmd = [os.path.join(self.rv_tester_path, "infra", "bzsim"), "run", "--rerun-test", "@smoke//:{}_verilator_shm".format(self.args.input_program)]
        self.container_run_cmd = [os.path.join(self.rv_tester_path, "infra", "podman-scripts", "container-run-tt")]
        self.common_sim_opts = ["--bazel-build-opt=--compilation_mode={}".format(args.compilation_mode), "--bazel-build-opt=--copt=-g", "--bazel-build-opt=--copt=-fno-omit-frame-pointer", "--debug"]
        if args.no_lsf:
            self.common_sim_opts.append("--no-lsf")
        self.common_plusargs = ["+nostandalone", "+whisper_client={}".format(args.whisper_client), "+eot=max_instr", "+max_cycle={}".format(self.args.max_cycle), "+max_instr={}".format(self.args.max_cycle - 20)]
        if args.no_cosim:
            self.common_plusargs.append("+nocosim")
        if not args.enable_logging:
            self.common_plusargs.extend(["+nowhisper_log", "+norvfi_log", "+nobridge_log"])
        if args.linux_time:
            self.common_plusargs.append("+sim_wrap=time")
        self.profiler_cmds = []


    # Colorful print to identify messages from this python script
    def print(self, input: str):
        print("\033[96m {}\033[00m".format(input))

    # Execute the profiler
    def run(self):
        self.print("Chosen profiler: {}, input program: {}, max cycles: {}".format(self.name(), self.args.input_program, self.args.max_cycle))
        self.setup()
        for cmd in self.profiler_cmds:
            self.print("Executing command: {}".format(cmd))
            output_file = None if cmd.output_file_name is None else open(cmd.output_file_name, "a")
            if cmd.output_file_name is not None:
                output_file.write("===== Command: {} =====\n".format(cmd))
                output_file.flush()
            subprocess.run(cmd.arg_list, stdout=output_file)
            if cmd.output_file_name is not None:
                output_file.close()
        self.teardown()

    # Prepares the profiler to be run(). This should involve appending to self.profiler_cmds
    @abstractmethod
    def setup(self):
        pass

    # Executes at the end of run(). This can involve print statements, closing output files, etc
    @abstractmethod
    def teardown(self):
        pass

    # Returns a human-readable name for this class.
    @abstractmethod
    def name():
        pass

    # Returns a description for this class.
    @abstractmethod
    def description():
        pass


    # Optionally adds subparser arguments to use when running this profiler
    @abstractmethod
    def add_arguments(parser: argparse.ArgumentParser):
        pass


def default_output_file(p: Profiler):
    return "{}-results.txt".format(p.name())

class GprofProfiler(Profiler):
    def setup(self):
        self.profiler_cmds.append(Command(arg_list=self.bzsim_run_cmd + ["--bazel-build-opt=--copt=-pg"] + self.common_sim_opts + ["--"] + self.common_plusargs))
        self.profiler_cmds.append(Command(arg_list=self.container_run_cmd + ["gprof", os.path.join(self.rv_tester_path, "bazel-bin/sw_testbench/sw_testbench_verilator"), "gmon.out"], output_file_name=default_output_file()))

    def teardown(self):
        self.print("See {} for the profiling results".format(default_output_file()))
        self.output_file.close()

    @staticmethod
    def name():
        return "gprof"

    @staticmethod
    def description():
        return "GNU profiler (gprof). Generates call graph of functions using CPU time"

    @staticmethod
    def add_arguments(parser):
        pass


class PerfProfiler(Profiler):
    def setup(self):
        # TODO(mboisvert): Do the compilation mode in a more clean way. This will be refactored anyways once we integrate this into bzsim.
        self.profiler_cmds.append(Command(arg_list=self.bzsim_run_cmd + self.common_sim_opts + ["--"] + self.common_plusargs + ["+sim_wrap=perf"] + ["+sim_wrap={}".format(cmd) for cmd in self.args.perf_cmd.split(" ")] + ["+sim_wrap=-o", "+sim_wrap=perf.data"]))

    def teardown(self):
        self.print('Run "{}" to analyze the output'.format(" ".join(self.container_run_cmd + ["perf", "report"])))

    @staticmethod
    def name():
        return "perf"

    @staticmethod
    def description():
        return "Linux perf profiler (https://perf.wiki.kernel.org/index.php/Main_Page)"

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--perf_cmd", type=str, default="record", help="Perf command used to collect info in perf.data (e.g. 'perf mem record -e')")


# Dummy class to run the same testbench without any profiling overhead
class NoProfiler(Profiler):
    def setup(self):
        self.profiler_cmds.append(Command(arg_list=self.bzsim_run_cmd + self.common_sim_opts + ["--"] + self.common_plusargs))

    def teardown(self):
        pass

    @staticmethod
    def name():
        return "profiling_disabled"

    @staticmethod
    def description():
        return "Runs rv_tester SW testbench without a profiler."

    @staticmethod
    def add_arguments(parser):
        pass

class GperftoolsProfiler(Profiler):
    def setup(self):
        self.output_file = open("{}-results.txt".format(self.name()), "w")
        timing_args = ["--container-run-opt=--env=CPUPROFILE_REALTIME=1", "--container-run-opt=--env=ITIMER_REAL=1"] if self.args.use_realtime else ["--container-run-opt=--env=CPUPROFILE_REALTIME=0"]
        self.profiler_cmds.append(Command(arg_list=self.bzsim_run_cmd + ["--container-run-opt=--env=CPUPROFILE_FREQUENCY=10000"] + timing_args + ["--container-run-opt=--env=CPUPROFILE=rv_tester.prof", "--bazel-build-opt=--linkopt=-Wl,-no-as-needed,-lprofiler"] + self.common_sim_opts + ["--"] + self.common_plusargs))
        self.profiler_cmds.append(Command(arg_list=self.container_run_cmd + ["pprof", "--dot", os.path.join(self.rv_tester_path, "bazel-bin/sw_testbench/sw_testbench_verilator"), "rv_tester.prof"], output_file_name="gperftools-cosim-results.txt"))
        self.profiler_cmds.append(Command(arg_list=self.container_run_cmd + ["pprof", "--dot", os.path.join(self.rv_tester_path, "/proj_risc_regr/asc/a0/user_regr/{}/rv_tester/bazel_output_base/execroot/rv_tester/bazel-out/k8-opt/bin/external/whisper/whisper".format(os.environ.get("USER"))), "whisper.prof"], output_file_name="gperftools-whisper-results.txt"))

    def teardown(self):
        self.print("See gperftools-cosim-results.txt and gperftools-whisper-results.txt for the profiling results. Use https://dreampuf.github.io/GraphvizOnline/ to visualize these graphs")

    @staticmethod
    def name():
        return "gperftools"

    @staticmethod
    def description():
        return "Google's suite of performance analysis tools, most notably pprof. Generates an output dot file containing a function call graph."

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--use_realtime", action="store_true", default=False, help="If true, use realtime for profiling. Otherwise, use CPU time")

# TODO(mboisvert): Figure out why the wall clock profiler isn't working when run from Python
class WallClockProfiler(Profiler):
    def setup(self):
        self.profiler_cmds.append(Command(arg_list=self.bazel_wrap_cmd + ["build", "@opensrc-wall_clock_profiler//:wall_clock_profiler"]))
        new_profiler_path = os.path.join(self.rv_tester_path, "wall_clock_profiler")
        self.profiler_cmds.append(Command(arg_list=["mv", "bazel-bin/external/wall_clock_profiler/wall_clock_profiler", new_profiler_path]))
        self.profiler_cmds.append(Command(arg_list=self.bzsim_run_cmd + self.common_sim_opts + ["--"] + self.common_plusargs + ["+sim_wrap={}".format(new_profiler_path), "+sim_wrap={}".format(self.args.samples_per_second)]))

    def teardown(self):
        self.print("See wcOut.txt for the profiling results")

    @staticmethod
    def name():
        return "wall_clock_profiler"

    @staticmethod
    def description():
        return "Executes a fork of https://github.com/jasonrohrer/wallClockProfiler."

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--samples_per_second", type=int, default=20, help="Number of times stack is sampled per second")


# Parses command-line args to construct a profiler
def construct_profiler_using_args(rv_tester_path: str):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    subparsers = parser.add_subparsers(dest="profiler")
    profiler_map = {}
    for profiler in [GprofProfiler, PerfProfiler, GperftoolsProfiler, WallClockProfiler, NoProfiler]:
        profiler_parser = subparsers.add_parser(profiler.name(), formatter_class=argparse.ArgumentDefaultsHelpFormatter, description=profiler.description())
        profiler.add_arguments(profiler_parser)
        profiler_map[profiler.name()] = profiler
    parser.add_argument("--max_cycle", type=int, default=999999, help="max number of cycles to run simulation")
    parser.add_argument("--input_program", type=str, default="infinite", help="name of the input program defined in rv_tester/test/sw_testbench/testlists/smoke.py")
    parser.add_argument("--whisper_client", type=str, choices=['lib', 'socket', 'shm'], default='lib', help="Mechanism of running/communicating with Whisper")
    parser.add_argument("--no_lsf", action="store_true", default=False, help="If true, run sim locally instead of on LSF")
    parser.add_argument("--no_cosim", action="store_true", default=False, help="If true, run without cosim")
    parser.add_argument("--enable_logging", action="store_true", default=False, help="If true, enable logging to cosim.log, dut_rvfi.log, iss_cosim.log, and iss_cmd.log")
    parser.add_argument("--linux_time", action="store_true", default=False, help="If true, wrap profiler with call to 'time'. E.g. 'time bazel-bin/...'")
    parser.add_argument("--compilation_mode", type=str, choices=['dbg', 'opt'], default='opt', help='Optimization level used for compiling testbench under profiler')

    args = parser.parse_args()
    if (args.profiler is None):
        raise Exception(parser.format_usage())
    return profiler_map[args.profiler](rv_tester_path, args)


# TODO(mboisvert): Figure out how to make it possible to run this script from directories other than rv_tester
def main():
    rv_tester_path = os.path.abspath(os.path.join(os.path.dirname(__file__),"../.."))
    profiler = construct_profiler_using_args(rv_tester_path)
    profiler.run()

if __name__ == "__main__":
    main()
