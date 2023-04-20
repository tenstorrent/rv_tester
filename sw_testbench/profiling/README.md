# Testbench Performance Analysis

To test the performance of `rv_tester`, we can run the SW testbench with a simple input file containing an infinite loop (`testlists/infinite.S`) for a sufficiently large number of cycles. Using various profilers, we can then evaluate the bottlenecks of `rv_tester`. The script `profile.py` provides an easy way to automate the process of running these profilers and dumping the output. E.g.:

```
[mboisvert@aus-rv-l-4 ~]$ cd path/to/rv_tester
[mboisvert@aus-rv-l-4 rv_tester]$ python3 sw_testbench/profiling/run_profiler.py --help
usage: run_profiler.py [-h] [--max_cycles MAX_CYCLES]
                       [--input_program INPUT_PROGRAM]
                       {gprof,perf,gperftools,wall_clock_profiler} ...

positional arguments:
  {gprof,perf,gperftools,wall_clock_profiler}

optional arguments:
  -h, --help            show this help message and exit
  --max_cycles MAX_CYCLES
                        max number of cycles to run simulation (default:
                        999999)
  --input_program INPUT_PROGRAM
                        path to source input file (default: /proj_risc/user_de
                        v/mboisvert/rv_tester/sw_testbench/testlists/infinite.
                        S)


[mboisvert@aus-rv-l-4 rv_tester]$ python3 sw_testbench/profiling/run_profiler.py gperftools --help
usage: run_profiler.py gperftools [-h] [--use_realtime]

optional arguments:
  -h, --help      show this help message and exit
  --use_realtime  If true, use realtime for profiling. Otherwise, use CPU time
                  (default: False)
[mboisvert@aus-rv-l-4 rv_tester]$ python3 sw_testbench/profiling/run_profiler.py gperftools --use_realtime
...
See gperftools-results.txt for the profiling results
[mboisvert@aus-rv-l-4 rv_tester]$ head gperftools-results.txt 
Using local file /proj_risc/user_dev/mboisvert/rv_tester/bazel-bin/sw_testbench/sw_testbench_verilator.
Using local file prof.out.
Total: 31617 samples
   30091  95.2%  95.2%    30091  95.2% __GI___nanosleep
    1231   3.9%  99.1%     1231   3.9% __libc_recv
     182   0.6%  99.6%      182   0.6% __GI___read
      19   0.1%  99.7%       19   0.1% __GI___waitpid
      17   0.1%  99.8%       17   0.1% __libc_send
      11   0.0%  99.8%       11   0.0% __libc_write
       9   0.0%  99.8%        9   0.0% __spawnix
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