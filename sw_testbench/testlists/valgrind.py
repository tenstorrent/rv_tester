#! /usr/bin/env python3

if __name__ == '__main__':
    print("cc_test_generator --name infinite -s //sw_testbench/testlists:infinite.S")
    print("risc_p_cores_test_executor --name infinite_verilator --test infinite --simulator verilator --tb sw_testbench:sw_testbench --run-arg +nostandalone +eot=max_instr +max_instr=1 +whisper_client=shm +sim_wrap=valgrind +sim_wrap=--track-origins=yes")
