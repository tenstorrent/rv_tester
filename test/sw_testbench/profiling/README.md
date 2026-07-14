# Testbench Performance Analysis

To test the performance of `rv_tester`, we can run the SW testbench with a simple input file containing an infinite loop (`testlists/infinite.S`) for a sufficiently large number of cycles. Using various profilers, we can then evaluate the bottlenecks of `rv_tester`. The script `profile.py` provides an easy way to automate the process of running these profilers and dumping the output. E.g.:

```
[user@host ~]$ cd path/to/rv_tester
[user@host rv_tester]$ python3 sw_testbench/profiling/run_profiler.py --help
usage: run_profiler.py [-h] [--max_cycles MAX_CYCLES]
                       [--input_program INPUT_PROGRAM]
                       {gprof,perf,gperftools,wall_clock_profiler} ...

positional arguments:
  {gprof,perf,gperftools,wall_clock_profiler}

optional arguments:
  -h, --help            show this help message and exit
  --max_cycle MAX_CYCLE
                        max number of cycles to run simulation (default:
                        999999)
  --input_program INPUT_PROGRAM
                        name of the input program defined in
                        rv_tester/test/sw_testbench/testlists/smoke.py (default:
                        infinite)
  --use_shm             If true, shared memory to communicate with whisper.
                        Otherwise, use sockets (default: False)
  --no_lsf              If true, run sim locally instead of on LSF (default:
                        False)
  --no_cosim            If true, run without cosim (default: False)
  --no_rvfi             If true, run without rvfi (default: False)
  --linux_time          If true, wrap profiler with call to 'time'. E.g. 'time
                        bazel-bin/...' (default: False)


[user@host rv_tester]$ python3 sw_testbench/profiling/run_profiler.py gperftools --help
usage: run_profiler.py gperftools [-h] [--use_realtime]

Google's suite of performance analysis tools, most notably pprof. Generates an
output dot file containing a function call graph.

optional arguments:
  -h, --help      show this help message and exit
  --use_realtime  If true, use realtime for profiling. Otherwise, use CPU time
                  (default: False)

[user@host rv_tester]$ python3 sw_testbench/profiling/run_profiler.py gperftools --use_realtime
...
See gperftools-results.txt for the profiling results
[user@host rv_tester]$ head gperftools-results.txt 
Using local file <path-to>/rv_tester/bazel-bin/sw_testbench/sw_testbench_verilator.
Using local file prof.out.
Dropping nodes with <= 76 samples; edges with <= 15 abs(samples)
digraph "<path-to>/rv_tester/bazel-bin/sw_testbench/sw_testbench_verilator; 15344 samples" {
node [width=0.375,height=0.25];
Legend [shape=box,fontsize=24,shape=plaintext,label="<path-to>/rv_tester/bazel-bin/sw_testbench/sw_testbench_verilator\lTotal samples: 15344\lFocusing on: 15344\lDropped nodes with <= 76 abs(samples)\lDropped edges with <= 15 samples\l"];
N1 [label="__libc_start_main\n0 (0.0%)\rof 15344 (100.0%)\r",shape=box,fontsize=8.0];
N2 [label="_start\n0 (0.0%)\rof 15344 (100.0%)\r",shape=box,fontsize=8.0];
N3 [label="main\n4 (0.0%)\rof 15343 (100.0%)\r",shape=box,fontsize=8.8];
N4 [label="Vtop___024root___eval\n30 (0.2%)\rof 15330 (99.9%)\r",shape=box,fontsize=10.2];
```


## Supported profilers

- [gprof](https://ftp.gnu.org/old-gnu/Manuals/gprof-2.9.1/html_mono/gprof.html): An application profiler which samples the number of CPU cycles that each function consumes and constructs a call graph of the program. This is how to build/run the testbench under `gprof`:
- [gperftools](https://github.com/gperftools/gperftools/tree/master/docs) is a collection of open-source performance tools released by Google. One of these tools, `pprof`, can be used to perform timing-based sampling of C++ programs.
- [Linux perf](https://www.brendangregg.com/perf.html) is a large suite of performance analysis tools developed for a wide array of userspace and kernel-level profiling use cases. This is an example of using the `perf record` and `perf report` commands to collect high-level profiling data on the testbench:
- [wall_clock_profiler](https://github.com/jasonrohrer/wallClockProfiler) is a debugger-based profiler which periodically snoops on a program to determine how long each function has run.


## References

For additional information regarding profilers, here are some useful references:

-  https://jsaxton.com/profiling-io-bound-applications/
-  https://stackoverflow.com/questions/11762372/profiling-a-possibly-i-o-bound-process-to-reduce-latency
-  https://stackoverflow.com/questions/375913/how-do-i-profile-c-code-running-on-linux/378024#378024