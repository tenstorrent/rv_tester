#include <cinttypes>

namespace rv_tester {

    class system_model {

        public:
            system_model() {}

    };

}

extern "C" void clock_update(std::uint64_t clock) { clock = clock; }
extern "C" void set_interrupt(std::uint8_t is_timer) { is_timer = is_timer; }
extern "C" rv_tester::system_model* rv_tester_system_model_new() {
    return new rv_tester::system_model();
}
