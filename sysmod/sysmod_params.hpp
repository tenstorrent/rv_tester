#pragma once
namespace{

    typedef struct cbo_inval_req_s{
        uint32_t cycle;
        uint64_t address;
        uint8_t txnid;
    } cbo_inval_req_t;

    typedef struct inval_load_s{
        uint32_t cycle;
        uint64_t address;
        uint64_t data;
        uint8_t size;
    } inval_load_t;

    typedef struct inval_crsp_s{
        uint64_t address;
        uint32_t hart;
    } inval_crsp_t;

    typedef struct inval_map_clean_s{
        uint64_t key;
        uint8_t txnid;
    } inval_map_clean_t;
}

