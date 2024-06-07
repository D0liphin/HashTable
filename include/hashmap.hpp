#pragma once

#include <stdint.h>
#include "buf.hpp"
#include "simd.hpp"
#include <emmintrin.h>
#include <limits>
#include <cstring>
#include <bitset>
#include <endian.h>

#if __BYTE_ORDER != __LITTLE_ENDIAN
#error bad arch
#endif

template <typename T> struct is_hashable
{
    static const bool value = false;

    static size_t hash(T const &)
    {
        std::runtime_error("not hashable");
    }
};

static_assert(std::numeric_limits<size_t>::digits == 64);

size_t byteshl(size_t n)
{
    return (n << 8) | ((n >> 56) & 0xff);
}

#define IMPL_HASHABLE_FOR_INTEGRAL(T)   \
    template <> struct is_hashable<T>   \
    {                                   \
        static const bool value = true; \
                                        \
        static size_t hash(T const &nt) \
        {                               \
            size_t n = nt;              \
            return n;                   \
        }                               \
    };

IMPL_HASHABLE_FOR_INTEGRAL(char);
IMPL_HASHABLE_FOR_INTEGRAL(unsigned char);
IMPL_HASHABLE_FOR_INTEGRAL(short);
IMPL_HASHABLE_FOR_INTEGRAL(unsigned short);
IMPL_HASHABLE_FOR_INTEGRAL(int);
IMPL_HASHABLE_FOR_INTEGRAL(unsigned int);
IMPL_HASHABLE_FOR_INTEGRAL(long);
IMPL_HASHABLE_FOR_INTEGRAL(unsigned long);

template <> struct is_hashable<std::string>
{
    static const bool value = true;

    static size_t hash(std::string const &str)
    {
        static_assert(alignof(max_align_t) > alignof(size_t));
        // std::string internally uses malloc, so this means we can now rely
        // on the buffer being size_t-aligned
        size_t len = str.size() / sizeof(size_t);
        size_t h = 0;
        size_t const *it = (size_t *)str.data();
        for (size_t i = 0; i < len; ++i) {
            h ^= is_hashable<size_t>::hash(*it);
            it++;
        }
        size_t shift = 0;
        for (size_t i = len * sizeof(size_t); i < str.size(); ++i) {
            h ^= is_hashable<size_t>::hash(((size_t)str[i]) << shift);
            shift += 8;
        }
        return h;
    }
};

using ctrlchunk_t = __m128i;
typedef movemask_t<ctrlchunk_t>::type ctrlmask_t;

/**
     * They are guaranteed to be readable as a `ctrlchunk_t`, which is some simd
     * Ctrl chunks attempt to pack the information about where an entry lies.
     * type that makes these operations actually fast.
     * 
     * The least significant byte is considered the 'first' ctrl-byte. 
     */
struct alignas(alignof(ctrlchunk_t)) CtrlChunk
{
    static const size_t NR_BYTES = sizeof(ctrlchunk_t);

    static constexpr char CTRL_EMPTY = -1;
    static constexpr char CTRL_DEL = -2;

    char bytes[NR_BYTES];

    CtrlChunk()
    {
        memset(bytes, 0, NR_BYTES);
    }

    CtrlChunk &operator=(CtrlChunk const &rhs)
    {
        memcpy(this, &rhs, sizeof(CtrlChunk));
        return *this;
    }

    inline ctrlchunk_t as_simd()
    {
        return *(ctrlchunk_t *)(this);
    }

    char &byte_at(size_t i)
    {
        return bytes[i];
    }

    static constexpr uint32_t CTRL_FIND_FOUND = 0;
    static constexpr uint32_t CTRL_FIND_NOTHING = 1;
    static constexpr uint32_t CTRL_FIND_MAYBE = 2;

    /**
     * Scan for `b`, starting at `start`. 
     * 
     * # Returns 
     * - `CTRL_FIND_FOUND` if we find this ctrl byte and set `offset` to the 
     *   location.
     * - `CTRL_FIND_MAYBE` if this ctrl chunk did not contain sufficient
     *   information to prove that `b` exists nowhere in the map. `offset` is
     *   not written to.
     * - `CTRL_FIND_NOTHING` if `b` exists nowhere in this map. `offset` is set
     *   to the index of the first subsequent empty cell
     */
    uint32_t find(char b, uint32_t start, uint32_t &offset)
    {
        ctrlmask_t const keep_mask = std::numeric_limits<ctrlmask_t>::max() << start;
        ctrlmask_t const eq_mask = simd<ctrlchunk_t>::movemask_eq(as_simd(), b) & keep_mask;
        if (eq_mask) {
            offset = __builtin_ctz(eq_mask);
            return CTRL_FIND_FOUND;
        }
        ctrlmask_t const empty_mask = simd<ctrlchunk_t>::movemask_eq(as_simd(), CTRL_EMPTY) &
                                      keep_mask;
        if (empty_mask) {
            offset = __builtin_ctz(empty_mask);
            return CTRL_FIND_NOTHING;
        }
        return CTRL_FIND_MAYBE;
    }

    /**
     * Find the first free byte, returning its offset from the start of this 
     * byte. Ignore the first `start` bytes. Returns `NR_BYTES` if there are no
     * empty cells.
     */
    uint32_t find_empty(uint32_t start)
    {
        ctrlmask_t const keep_mask = std::numeric_limits<ctrlmask_t>::max() << start;
        ctrlmask_t const empty_mask = simd<ctrlchunk_t>::movemask_eq(as_simd(), CTRL_EMPTY) &
                                      keep_mask;
        return empty_mask ? __builtin_ctz(empty_mask) : NR_BYTES;
    }

    /**
     * Get a mask with a `1` at every location where there is a non-deleted
     * entry present.
     */
    ctrlmask_t present_mask()
    {
        ctrlmask_t const empty_mask = simd<ctrlchunk_t>::movemask_eq(as_simd(), CTRL_EMPTY);
        ctrlmask_t const del_mask = simd<ctrlchunk_t>::movemask_eq(as_simd(), CTRL_DEL);
        return ~(del_mask | empty_mask);
    }
};
static_assert(sizeof(CtrlChunk) == sizeof(ctrlchunk_t));

template <typename Key, typename Val> struct HashTbl
{
    static_assert(is_hashable<Key>::value, "Key must be hashable");

    using Self = HashTbl<Key, Val>;

private:
    /*
    +-----------+
    | ctrl      |
    +-----------+
    | entries   |
    +-----------+
    */
    FlatBuf buf;
    size_t max_nr_entries;
    /** used to calculate load factor */
    size_t nr_used;

    static const size_t BUF_ALIGNMENT = alignof(ctrlchunk_t);

public:
    struct Entry
    {
        uint64_t hash;
        Key key;
        Val val;

        Entry(Key key, Val val)
            : key(key)
            , val(val)
        {
            hash = is_hashable<Key>::hash(key);
        }

        char h7()
        {
            return (char)(hash & 0b1111111);
        }
    };

    HashTbl()
        : buf(FlatBuf())
        , nr_used(0)
    {
    }

    static Self with_capacity(size_t capacity)
    {
        auto self = Self();
        self.buf.grow(Layout(0, 0), Layout(capacity, BUF_ALIGNMENT));
        return self;
    }

    ~HashTbl()
    {
        buf.dealloc();
    }

    HashTbl(HashTbl &)
    {
        throw std::runtime_error("Not yet implemented!");
    }

    HashTbl &operator=(HashTbl &)
    {
        throw std::runtime_error("Not yet implemented!");
    }

    HashTbl(HashTbl &&)
    {
        throw std::runtime_error("Not yet implemented!");
    }

    HashTbl &operator=(HashTbl &&)
    {
        throw std::runtime_error("Not yet implemented!");
    }

    static inline size_t ctrlchunk_buf_size()
    {
        // We need to make this assumption for our calculation to make sense --
        // that the byte directly after our ctrl chunks is ctrlchunk_t-aligned.
        static_assert(alignof(ctrlchunk_t) == sizeof(ctrlchunk_t));
        size_t padding_sz =
            alignof(ctrlchunk_t) > alignof(Entry) ? 0 : alignof(Entry) - alignof(ctrlchunk_t);
        return sizeof(ctrlchunk_t) + padding_sz;
    }

    size_t buf_size()
    {
        return ctrlchunk_buf_size() + sizeof(Entry) * max_nr_entries;
    }

    void grow()
    {
        Self new_tbl;
        new_tbl.buf.grow(Layout(0, 0), Layout(buf_size() * 2, BUF_ALIGNMENT));
        new_tbl.max_nr_entries = max_nr_entries * 2;
        throw std::runtime_error("not yet implemented");
    }

    CtrlChunk *ctrlchunks()
    {
        return (CtrlChunk *)buf.data;
    }

    void insert_unchecked(size_t idx, Entry e)
    {
        throw std::runtime_error("nye");
    }

    /**
     * Insert, without checkign the ctrl bytes first, just go straight to idx 
     * and start 
     */
    void insert_first_available(size_t idx, Entry e)
    {
        throw std::runtime_error("nye");
    }

    /**
     * Insert a key-value pair into the hash-table, overriding an existing value
     * if there is one.
     */
    void insert(Key key, Val val)
    {
        Entry e = Entry(std::move(key), std::move(val));
        size_t idx = e.hash % max_nr_entries;
        uint32_t skip = idx % CtrlChunk::NR_BYTES;
        size_t ctrlchunk_idx = idx / CtrlChunk::NR_BYTES;
        while (true) {
            uint32_t offset;
            switch ((ctrlchunks() + ctrlchunk_idx)->find(e.h7(), skip, offset)) {
            case CtrlChunk::CTRL_FIND_FOUND:
                insert_first_available(ctrlchunk_idx * CtrlChunk::NR_BYTES + offset, std::move(e));
                return;
            case CtrlChunk::CTRL_FIND_NOTHING:
                insert_unchecked(ctrlchunk_idx * CtrlChunk::NR_BYTES + offset, std::move(e));
                return;
            case CtrlChunk::CTRL_FIND_MAYBE:
                ctrlchunk_idx = (ctrlchunk_idx + 1) % max_nr_entries;
                skip = 0;
                break;
            };
        }
        throw std::runtime_error("Not yet implemented!");
    }

    /**
     * Get a pointer to the value at this key, or `nullptr` if it does not 
     * exist.
     */
    Val *get(Key const &)
    {
        throw std::runtime_error("Not yet implemented!");
    }
};