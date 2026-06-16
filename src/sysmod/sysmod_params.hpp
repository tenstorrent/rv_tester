#pragma once
namespace{

    typedef struct cbo_inval_req_s{
        uint32_t cycle;
        uint64_t address;
        uint8_t txnid;
        uint8_t hart;
    } cbo_inval_req_t;

    typedef struct inval_load_s{
        uint32_t cycle;
        uint64_t address;
        uint64_t data;
        uint8_t size;
        bool amo;
    } inval_load_t;

    typedef struct inval_crsp_s{
        uint64_t address;
        uint32_t hart;
    } inval_crsp_t;

    typedef struct inval_map_clean_s{
        uint64_t key;
        uint16_t txn_key;
    } inval_map_clean_t;

    struct cbo_inval_nomcm_s {
        uint64_t address;
    };
}
