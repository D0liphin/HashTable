#pragma once

#include <emmintrin.h>
#include <cstdint>
#include <limits>
#include <cstring>

template <size_t NR_BITS> struct unsigned_int
{
};

template <> struct unsigned_int<16>
{
    using type = uint16_t;
};

template <typename T> struct movemask_t
{
    using type = typename unsigned_int<sizeof(T)>::type;
};

template <typename T> struct usimd
{
};

/**
 * - x86 is little-endian, so our pointer-dereference bitcasts are all 
 *   just as-you-would-expect truncations.  
 */
template <> struct usimd<__m128i>
{
    using movemask_t = typename movemask_t<__m128i>::type;

    static __m128i splat_i8(char b)
    {
        return _mm_set1_epi8(b);
    }

    static __m128i cmpeq_i8(__m128i a, __m128i b)
    {
        return _mm_cmpeq_epi8(a, b);
    }

    static movemask_t movemask_i8(__m128i i)
    {
        // dw, this is as you would expect,
        // pmovmskb        eax, xmm0
        // ret
        int mm = _mm_movemask_epi8(i);
        movemask_t ret;
        memcpy(&ret, &mm, sizeof(movemask_t));
        return ret;
    }
};

template <typename T> struct simd
{
    using movemask_t = typename usimd<T>::movemask_t;

    /**
     * Construct a mask, where each high bit represents an equality match
     * of the byte `b`.
     */
    static movemask_t movemask_eq(T v, char b)
    {
        T const splat = usimd<T>::splat_i8(b);
        T const eqmask = usimd<T>::cmpeq_i8(splat, v);
        return usimd<T>::movemask_i8(eqmask);
    }
};