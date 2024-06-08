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
    static constexpr bool value = false;

    static size_t hash(T const &)
    {
        throw std::runtime_error("not hashable");
    }
};

template <typename T> struct is_trivially_equatable
{
    static constexpr bool value = false;
};

static_assert(std::numeric_limits<size_t>::digits == 64);

size_t byteshl(size_t n)
{
    return (n << 8) | ((n >> 56) & 0xff);
}

#define IMPL_HASHABLE_FOR_INTEGRAL(T)            \
    template <> struct is_hashable<T>            \
    {                                            \
        static constexpr bool value = true;      \
                                                 \
        static size_t hash(T const &nt)          \
        {                                        \
            size_t n = nt;                       \
            return n;                            \
        }                                        \
    };                                           \
                                                 \
    template <> struct is_trivially_equatable<T> \
    {                                            \
        static constexpr bool value = true;      \
    }

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
    static constexpr bool value = true;

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
    static constexpr size_t NR_BYTES = sizeof(ctrlchunk_t);
    static_assert(sizeof(ctrlchunk_t) == alignof(ctrlchunk_t));
    // Thus sizeof(ctrlchunk_t) is a power of 2

    static constexpr char CTRL_EMPTY = -1;
    static constexpr char CTRL_DEL = -2;

    char bytes[NR_BYTES];

    CtrlChunk()
    {
        memset(bytes, 0, NR_BYTES);
    }

    CtrlChunk(CtrlChunk const &rhs)
    {
        *(ctrlchunk_t *)this = rhs.as_simd();
    }

    CtrlChunk &operator=(CtrlChunk const &rhs)
    {
        *(ctrlchunk_t *)this = rhs.as_simd();
        return *this;
    }

    inline ctrlchunk_t as_simd() const
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
     *   to the index of the first subsequent empty slot
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
     * empty slots.
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
    uint8_t *buf;
    size_t max_nr_entries;
    /** used to calculate load factor */
    size_t nr_used;

    static const size_t BUF_ALIGNMENT = alignof(ctrlchunk_t);

    char h7(size_t hash)
    {
        return (char)(hash & 0b1111111);
    }

    bool cmp_keys(size_t hash, Key const &key, size_t other_hash, Key const &other_key)
    {
        // should just optimize away
        if (is_trivially_equatable<Key>::value) {
            return hash == other_hash && key == other_key;
        } else {
            return key == other_key;
        }
    }

public:
    struct Entry
    {
        size_t hash;
        Key key;
        Val val;

        Entry()
            : hash()
            , key()
            , val()
        {
        }

        Entry(size_t hash, Key key, Val val)
            : hash(hash)
            , key(std::move(key))
            , val(std::move(val))
        {
        }

        Entry(const Entry &other) = delete;

        Entry &operator=(const Entry &other) = delete;
        // {
        //     if (this != &other) {
        //         this.hash = other.hash;
        //         this.key = other.key;
        //         this.val = other.val;
        //     }
        // }

        Entry(Entry &&other) noexcept
            : hash(other.hash)
            , key(std::move(other.key))
            , val(std::move(other.val))
        {
        }

        Entry &operator=(Entry &&other) noexcept
        {
            hash = other.hash;
            key = std::move(other.key);
            val = std::move(other.val);
            return *this;
        }

        ~Entry() = default;
    };

    HashTbl()
        : buf(nullptr)
        , nr_used(0)
    {
    }

    static Self with_capacity(size_t capacity)
    {
        auto self = Self();
        // alignup
        self.max_nr_entries = (capacity / CtrlChunk::NR_BYTES + 1) * CtrlChunk::NR_BYTES;
        std::cout << "alloc buf size = " << self.buf_size() << std::endl;
        if (posix_memalign((void **)&self.buf, BUF_ALIGNMENT, self.buf_size())) {
            throw std::runtime_error("OOM");
        }
        memset(self.buf, CtrlChunk::CTRL_EMPTY, self.max_nr_entries);
        return self;
    }

    ~HashTbl()
    {
        if (buf) free(buf);
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

    size_t ctrlchunk_buf_size()
    {
        // We need to make this assumption for our calculation to make sense --
        // that the byte directly after our ctrl chunks is ctrlchunk_t-aligned.
        // alignof(ctrlchunk_t) == sizeof(ctrlchunk_t)
        size_t padding_sz =
            alignof(ctrlchunk_t) > alignof(Entry) ? 0 : alignof(Entry) - alignof(ctrlchunk_t);
        return sizeof(ctrlchunk_t) * max_nr_entries + padding_sz;
    }

    size_t buf_size()
    {
        return ctrlchunk_buf_size() + sizeof(Entry) * max_nr_entries;
    }

    void grow()
    {
        throw std::runtime_error("not yet implemented");
    }

    CtrlChunk *ctrlchunks_buf()
    {
        return (CtrlChunk *)buf;
    }

    Entry *entries_buf()
    {
        return (Entry *)(buf + ctrlchunk_buf_size());
    }

    void insert_unchecked(size_t idx, Entry e)
    {
        throw std::runtime_error("nye");
    }

    /**
     * Get the slot where we can insert something with the provided `key`. 
     * 
     * # Returns
     * - `true` if the slot is empty
     * - `false` if it is occupied
     */
    bool get_slot(size_t h, Key const &key, Entry *&slot, char *&ctrl_slot)
    {
        if (max_nr_entries == 0) grow();

        Entry *entries = entries_buf();
        CtrlChunk *ctrlchunks = ctrlchunks_buf();

        // Just memoize some stuff for readability mostly
        size_t max_nr_ctrlchunks = max_nr_entries / CtrlChunk::NR_BYTES;
        size_t entry_idx = h % max_nr_entries;
        uint32_t ctrlchunk_idx = entry_idx / CtrlChunk::NR_BYTES;
        uint32_t ctrlbyte_offset = entry_idx % CtrlChunk::NR_BYTES;
        std::cout << "entry_idx = " << entry_idx << std::endl;

        CtrlChunk ctrlchunk = *(ctrlchunks + ctrlchunk_idx);
        ctrlmask_t keep_mask = std::numeric_limits<ctrlmask_t>::max() << ctrlbyte_offset;
        ctrlmask_t hit_mask = simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), h7(h)) &
                              keep_mask;
        ctrlmask_t empty_mask =
            simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), CtrlChunk::CTRL_EMPTY) & keep_mask;
        while (true) {
            char the_bin;
            std::cin >> the_bin;
            std::cout << "keep_mask  = " << std::bitset<16>(keep_mask) << std::endl;
            std::cout << "hit_mask   = " << std::bitset<16>(hit_mask) << std::endl;
            std::cout << "empty_mask = " << std::bitset<16>(empty_mask) << std::endl;
            std::cout << "ctrlchunk_idx = " << ctrlchunk_idx << std::endl;
            std::cout << "ctrlbyte_offset = " << ctrlbyte_offset << std::endl;

            // ctz appears to be faster than alternative methods TODO: test
            uint32_t empty_mask_tz = __builtin_ctz(empty_mask);
            if (!hit_mask || empty_mask_tz < __builtin_ctz(hit_mask)) {
                // If we have no matches, but there is an empty slot, we just
                // put it there
                if (empty_mask) {
                    ctrlbyte_offset = empty_mask_tz;
                    std::cout << "found empty slot at "
                              << (ctrlchunk_idx * CtrlChunk::NR_BYTES + ctrlbyte_offset)
                              << std::endl;
                    size_t i = ctrlchunk_idx * CtrlChunk::NR_BYTES + ctrlbyte_offset;
                    slot = entries + i;
                    ctrl_slot = (char *)ctrlchunks_buf() + i;
                    return true;
                }
                // If we have no matches and there is no empty slot, we must
                // continue probing in subsequent chunks
                ctrlchunk_idx = (ctrlchunk_idx + 1) % max_nr_ctrlchunks;
                ctrlchunk = ctrlchunks[ctrlchunk_idx];
                hit_mask = simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), h7(h));
                empty_mask =
                    simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), CtrlChunk::CTRL_EMPTY);
                continue;
            }
            // We have some kind of hit that we need to check is a complete hit
            ctrlbyte_offset = __builtin_ctz(hit_mask);
            size_t i = ctrlchunk_idx * CtrlChunk::NR_BYTES + ctrlbyte_offset;
            Entry *entry = entries + i;
            if (cmp_keys(h, key, entry->hash, entry->key)) {
                slot = entries + i;
                ctrl_slot = (char *)ctrlchunks_buf() + i;
                return false;
            }
            hit_mask &= ~((ctrlmask_t)1 << ctrlbyte_offset);
        }
    }

    /**
     * Insert a key-value pair into the hash-table, overriding an existing value
     * if there is one. 
     * 
     * # Returns
     * A pointer to the value
     */
    Val *insert(Key key, Val val)
    {
        size_t h = is_hashable<Key>::hash(key);
        Entry *slot;
        char *ctrl_slot;
        get_slot(h, key, slot, ctrl_slot);

        new (slot) Entry(h, key, val);
        *ctrl_slot = h7(h);

        return &(slot->val);
    }

    /**
     * Get a pointer to the value at this key, or `nullptr` if it does not 
     * exist.
     */
    Val *get(Key const &key)
    {
        size_t h = is_hashable<Key>::hash(key);
        Entry *slot;
        char *ctrl_slot;
        bool empty = get_slot(h, key, slot, ctrl_slot);
        
        if (empty) return nullptr;
        return &slot->val;
    }
};

//
// std::cout << "buf = " << (void *)buf << ", buf size = " << buf_size() << std::endl;
// std::cout << (void *)slot << std::endl;