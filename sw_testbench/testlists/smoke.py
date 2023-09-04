#! /usr/bin/env python3

if __name__ == '__main__':
    print("cc_test_generator --name infinite -s //sw_testbench/testlists:infinite.S")
    print("risc_p_cores_test_executor --name infinite_1c_verilator --test infinite --simulator verilator --tb sw_testbench:sw_1c_tb --run-arg +nostandalone +eot=max_instr +max_instr=1")
    print("risc_p_cores_test_executor --name infinite_2c_verilator --test infinite --simulator verilator --tb sw_testbench:sw_2c_tb --run-arg +nostandalone +eot=max_instr +max_instr=2")
