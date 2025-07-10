#pragma once

// smc_write_pkt struct definition
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t size;
} smc_write_pkt;

// smc_read_pkt struct definition
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t size;
    uint64_t resp;
} smc_read_pkt;

enum smc_txn_type {
    SMC_READ,
    SMC_WRITE
};
typedef struct {
    smc_txn_type req_type;
    uint64_t addr;
} smc_req_pkt;