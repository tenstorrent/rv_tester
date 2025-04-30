#pragma once

namespace rv_tester{

    // used to signal termination for any monitors and checkers to shut off
    // namely cosim can go way ahead on zebu after the test is trying to terminate
    struct terminate_called {};
    struct terminate_called_mem_checks {};
    struct snoop_mem { bool done = false;};
    struct snoop_addrs_eot { std::queue<uint64_t> address;};

    // for handlers which want to terminate immediately
    struct terminate_called_fast {};

    // used to signal when whisper is ready for cosim calls
    struct whisper_connected {};

    // used to signal when actual test starts
    struct actual_test_start{};
    struct started_t {};
}
