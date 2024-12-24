#pragma once

typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t size;
} smc_write_pkt;

typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t size;
} smc_read_pkt;