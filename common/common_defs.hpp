#pragma once

#include <string>
#include <cstdint>
#include "cvm/messenger.hpp"

struct ras_backdoor_write_t {
    uint64_t addr ;
    uint64_t data ;
};
struct ras_backdoor_read_t {
    uint64_t addr ;
    uint64_t data ;
};
struct ras_backdoor_read_req_t {
    uint64_t addr ;
};