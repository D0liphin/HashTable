// #include "assert.h"
// #include "hashmap.hpp"
// #include "simd.hpp"
// #include <emmintrin.h>

// void ctrlchunk_finds()
// {
//     CtrlChunk chunk;
//     chunk.byte_at(4) = 0xab;
//     chunk.byte_at(5) = 0xbb;
//     for (size_t i = 7; i < 16; ++i) {
//         chunk.byte_at(i) = 0x12;
//     }
//     uint32_t out;
//     ASSERT(chunk.find(0xbb, 0, out) == CtrlChunk::CTRL_FIND_FOUND);
//     ASSERT(out == 5);
//     ASSERT(chunk.find(0xbb, 7, out) == CtrlChunk::CTRL_FIND_MAYBE);
// }

// void movemask_eq_m128i()
// {
//     char const N = 0xb2;
//     __m128i n = _mm_set_epi8(N, ~N, ~N, ~N, N, N, ~N, ~N, N, N, N, ~N, N, N, ~N, ~N);
//     auto mask = simd<__m128i>::movemask_eq(n, N);
//     ASSERT(mask == uint16_t(0b1000110011101100u));
// }

// int main()
// {
//     RUNTEST(ctrlchunk_finds);
//     RUNTEST(movemask_eq_m128i);
//     return 0;
// }

int main() {
    return 0;
}