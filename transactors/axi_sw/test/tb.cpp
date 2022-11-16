#include <cinttypes>
#include <cstdio>
#include <cassert>

extern "C" void get_stim(
        std::uint32_t  clocks,
        std::uint8_t*  finish,
        std::uint8_t*  reset_n,
        std::uint8_t*  aw_valid,
        std::uint64_t* aw_addr,
        std::uint8_t*  aw_len,
        std::uint8_t*  aw_size,
        std::uint8_t*  aw_burst,
        std::uint8_t*  ar_valid,
        std::uint64_t* ar_addr,
        std::uint8_t*  ar_len,
        std::uint8_t*  ar_size,
        std::uint8_t*  ar_burst,
        std::uint8_t*  w_valid,
        std::uint8_t   w_data[64],
        std::uint8_t   w_strb[8],
        std::uint8_t*  w_last,
        std::uint8_t   r_valid,
        std::uint8_t   r_id,
        std::uint8_t   r_data[64],
        std::uint8_t   r_last
) {
    *reset_n = clocks > 15;
    *finish = 0;

    if (clocks == 21) {
        *aw_valid = 1;
        *aw_addr = 0xabcd00;
        *aw_len  = 0;
        *aw_size = 6;
    } else {
        *aw_valid = 0;
    }

    if (clocks == 22) {
        *w_valid = 1;
        w_data[0] = 0xee;
        w_data[63] = 0xaa;
        for(int i = 1; i < 63; i++) w_data[i] = i;
        w_strb[0] = 0xff;
        w_strb[7] = 0xff;
        for (int i = 1; i < 7; i++) w_strb[i] = 0;
        *w_last   = 1;
    } else {
        *w_valid = 0;
    }

    if (clocks == 23) {
        *ar_valid = 1;
        *ar_addr  = 0xabcd00;
        *ar_len   = 0;
        *ar_size  = 6;
    } else {
        *ar_valid = 0;
    }

    if (r_valid) {
        printf("tb received read data %" PRIx8 "..%" PRIx8 "\n", r_data[0], r_data[63]);
        assert(r_data[0] == 0xee);
        assert(r_data[63] == 0xaa);
        for(int i = 1; i < 63; i++) assert( r_data[i] == ((i < 8 || i >= 56 ) ? i : 0 ));
        *finish = 1;
    }


}
