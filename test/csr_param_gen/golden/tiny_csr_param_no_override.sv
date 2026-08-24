// SystemVerilog CSR Defines Package
// Auto-generated from CSR specification

package csr_map_pkg;

// CSR Address Defines
parameter logic [11:0] TIME_ADDR = 12'hC01;
parameter logic [11:0] UTIME_ADDR = 12'hD01;
parameter logic [11:0] MY_CTL_ADDR = 12'h7C0;
parameter logic [11:0] C_DBG_CTL_ADDR = 12'hBC0;
parameter logic [11:0] MISA_ADDR = 12'h301;
parameter logic [11:0] C_SCRATCH_ADDR = 12'h5C0;

// CSR Size Defines
parameter int TIME_SIZE = 64;
parameter int UTIME_SIZE = 64;
parameter int MY_CTL_SIZE = 64;
parameter int C_DBG_CTL_SIZE = 64;
parameter int MISA_SIZE = 64;
parameter int C_SCRATCH_SIZE = 64;

// TIME CSR Field Defines
parameter int TIME_COUNT_MSB = 63;
parameter int TIME_COUNT_LSB = 0;
parameter int TIME_COUNT_WIDTH = 64;
parameter logic [63:0] TIME_COUNT_RESET = 64'h0000000000000000;
parameter logic [63:0] TIME_COUNT_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string TIME_COUNT_SW_TYPE = "WARL";


// UTIME CSR Field Defines
parameter int UTIME_COUNT_MSB = 63;
parameter int UTIME_COUNT_LSB = 0;
parameter int UTIME_COUNT_WIDTH = 64;
parameter logic [63:0] UTIME_COUNT_RESET = 64'h0000000000000000;
parameter logic [63:0] UTIME_COUNT_MASK = 64'hFFFFFFFFFFFFFFFF;
parameter string UTIME_COUNT_SW_TYPE = "WARL";


// MY_CTL CSR Field Defines
parameter int MY_CTL_CFG_SEL_MSB = 11;
parameter int MY_CTL_CFG_SEL_LSB = 4;
parameter int MY_CTL_CFG_SEL_WIDTH = 8;
parameter logic [7:0] MY_CTL_CFG_SEL_RESET = 8'h00;
parameter logic [63:0] MY_CTL_CFG_SEL_MASK = 64'h0000000000000FF0;
parameter string MY_CTL_CFG_SEL_SW_TYPE = "RW";

parameter int MY_CTL_MODE_MSB = 1;
parameter int MY_CTL_MODE_LSB = 0;
parameter int MY_CTL_MODE_WIDTH = 2;
parameter logic [1:0] MY_CTL_MODE_RESET = 2'h0;
parameter logic [63:0] MY_CTL_MODE_MASK = 64'h0000000000000003;
parameter string MY_CTL_MODE_SW_TYPE = "WARL";


// C_DBG_CTL CSR Field Defines
parameter int C_DBG_CTL_EN_MSB = 0;
parameter int C_DBG_CTL_EN_LSB = 0;
parameter int C_DBG_CTL_EN_WIDTH = 1;
parameter logic [0:0] C_DBG_CTL_EN_RESET = 1'h1;
parameter logic [63:0] C_DBG_CTL_EN_MASK = 64'h0000000000000001;
parameter string C_DBG_CTL_EN_SW_TYPE = "WARL";

parameter int C_DBG_CTL__2ND_EN_MSB = 5;
parameter int C_DBG_CTL__2ND_EN_LSB = 5;
parameter int C_DBG_CTL__2ND_EN_WIDTH = 1;
parameter logic [0:0] C_DBG_CTL__2ND_EN_RESET = 1'h0;
parameter logic [63:0] C_DBG_CTL__2ND_EN_MASK = 64'h0000000000000020;
parameter string C_DBG_CTL__2ND_EN_SW_TYPE = "WARL";

parameter int C_DBG_CTL_SEL MODE_MSB = 3;
parameter int C_DBG_CTL_SEL MODE_LSB = 1;
parameter int C_DBG_CTL_SEL MODE_WIDTH = 3;
parameter logic [2:0] C_DBG_CTL_SEL MODE_RESET = 3'h1;
parameter logic [63:0] C_DBG_CTL_SEL MODE_MASK = 64'h000000000000000E;
parameter string C_DBG_CTL_SEL MODE_SW_TYPE = "WARL";

parameter int C_DBG_CTL_GHOST_MSB = 3;
parameter int C_DBG_CTL_GHOST_LSB = 1;
parameter int C_DBG_CTL_GHOST_WIDTH = 3;
parameter logic [2:0] C_DBG_CTL_GHOST_RESET = 3'h0;
parameter logic [63:0] C_DBG_CTL_GHOST_MASK = 64'h000000000000000E;
parameter string C_DBG_CTL_GHOST_SW_TYPE = "WARL";


// MISA CSR Field Defines
parameter int MISA_MXL_MSB = 63;
parameter int MISA_MXL_LSB = 60;
parameter int MISA_MXL_WIDTH = 4;
parameter logic [3:0] MISA_MXL_RESET = 4'h0;
parameter logic [63:0] MISA_MXL_MASK = 64'hF000000000000000;
parameter string MISA_MXL_SW_TYPE = "WARL";

parameter int MISA_EXTENSIONS_MSB = 25;
parameter int MISA_EXTENSIONS_LSB = 0;
parameter int MISA_EXTENSIONS_WIDTH = 26;
parameter logic [25:0] MISA_EXTENSIONS_RESET = 26'h0000141;
parameter logic [63:0] MISA_EXTENSIONS_MASK = 64'h0000000003FFFFFF;
parameter string MISA_EXTENSIONS_SW_TYPE = "WARL";


// C_SCRATCH CSR Field Defines
parameter int C_SCRATCH_DATA_MSB = 31;
parameter int C_SCRATCH_DATA_LSB = 0;
parameter int C_SCRATCH_DATA_WIDTH = 32;
parameter logic [31:0] C_SCRATCH_DATA_RESET = 32'h00000000;
parameter logic [63:0] C_SCRATCH_DATA_MASK = 64'h00000000FFFFFFFF;
parameter string C_SCRATCH_DATA_SW_TYPE = "RW";

parameter int C_SCRATCH_TAG_MSB = 35;
parameter int C_SCRATCH_TAG_LSB = 32;
parameter int C_SCRATCH_TAG_WIDTH = 4;
parameter logic [3:0] C_SCRATCH_TAG_RESET = 4'h0;
parameter logic [63:0] C_SCRATCH_TAG_MASK = 64'h0000000F00000000;
parameter string C_SCRATCH_TAG_SW_TYPE = "WARL";


// CSR Reset Value Defines
parameter logic [63:0] TIME_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] UTIME_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] MY_CTL_RESET_VAL = 64'h0000000000000000;
parameter logic [63:0] C_DBG_CTL_RESET_VAL = 64'h0000000000000027;
parameter logic [63:0] MISA_RESET_VAL = 64'h8000000000000141;
parameter logic [63:0] C_SCRATCH_RESET_VAL = 64'h0000000F00000000;

// Utility Macros for Field Access
// Extract field value from CSR value
`define CSR_FIELD_GET(csr_val, field_msb, field_lsb) \
    ((csr_val >> field_lsb) & ((1 << (field_msb - field_lsb + 1)) - 1))

// Set field value in CSR value
`define CSR_FIELD_SET(csr_val, field_val, field_msb, field_lsb) \
    ((csr_val & ~(((1 << (field_msb - field_lsb + 1)) - 1) << field_lsb)) | \
     ((field_val & ((1 << (field_msb - field_lsb + 1)) - 1)) << field_lsb))

// Field Access Macros
`define TIME_COUNT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define TIME_COUNT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define UTIME_COUNT_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 0)

`define UTIME_COUNT_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 0)

`define MY_CTL_CFG_SEL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 11, 4)

`define MY_CTL_CFG_SEL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 11, 4)

`define MY_CTL_MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 1, 0)

`define MY_CTL_MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 1, 0)

`define C_DBG_CTL_EN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 0, 0)

`define C_DBG_CTL_EN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 0, 0)

`define C_DBG_CTL__2ND_EN_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 5, 5)

`define C_DBG_CTL__2ND_EN_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 5, 5)

`define C_DBG_CTL_SEL MODE_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 1)

`define C_DBG_CTL_SEL MODE_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 1)

`define C_DBG_CTL_GHOST_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 3, 1)

`define C_DBG_CTL_GHOST_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 3, 1)

`define MISA_MXL_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 63, 60)

`define MISA_MXL_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 63, 60)

`define MISA_EXTENSIONS_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 25, 0)

`define MISA_EXTENSIONS_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 25, 0)

`define C_SCRATCH_DATA_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 31, 0)

`define C_SCRATCH_DATA_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 31, 0)

`define C_SCRATCH_TAG_GET(csr_val) \
    `CSR_FIELD_GET(csr_val, 35, 32)

`define C_SCRATCH_TAG_SET(csr_val, field_val) \
    `CSR_FIELD_SET(csr_val, field_val, 35, 32)

// CSR Alias Defines
// utime is an alias of time
parameter logic [11:0] UTIME_ALIAS_OF_ADDR = TIME_ADDR;

endpackage : csr_map_pkg
