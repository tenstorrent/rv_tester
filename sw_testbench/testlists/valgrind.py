#! /usr/bin/env python3

if __name__ == '__main__':
    print("cc_test_generator --name infinite -s //sw_testbench/testlists:infinite.S")
    for whisper_client in ["socket", "shm"]:
        print("risc_p_cores_test_executor --name infinite_verilator_{whisper_client} --test infinite --simulator verilator --tb sw_testbench:sw_testbench --run-arg +nostandalone +eot=max_instr +max_instr=1 +whisper_client={whisper_client} +sim_wrap=valgrind +sim_wrap=--track-origins=yes".format(whisper_client=whisper_client))