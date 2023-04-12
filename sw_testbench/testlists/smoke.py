#! /usr/bin/env python3

if __name__ == '__main__':
    print("cc_test_generator --name nop -s //sw_testbench/testlists:nop.S")
    print("risc_p_cores_test_executor --name nop_verilator --test nop --simulator verilator --dbg dbg --tb sw_testbench:sw_testbench --run-arg +nostandalone +eot=max_instr +max_instr=1")
