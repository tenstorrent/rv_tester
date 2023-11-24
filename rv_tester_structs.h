#pragma once

namespace rv_tester{

    // used to signal termination for any monitors and checkers to shut off
    // namely cosim can go way ahead on zebu after the test is trying to terminate
    struct terminate_called {};

}
