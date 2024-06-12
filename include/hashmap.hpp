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

    inline static ctrlmask_t mask_ctz(ctrlmask_t n)
    {
        return unsigned_int<std::numeric_limits<ctrlmask_t>::digits>::ctz(n);
    }
};
static_assert(sizeof(CtrlChunk) == sizeof(ctrlchunk_t));

/**
 * Align `n` up to the nearest power of `pow2`. UB if `pow2` is not a power of
 * 2. 
 */
size_t inline alignup(size_t n, size_t pow2)
{
    size_t mask = pow2 - 1;
    return (n + mask) & ~mask;
}

template <typename Key, typename Val> struct HashTbl
{
    static_assert(is_hashable<Key>::value, "Key must be hashable");
    using Self = HashTbl<Key, Val>;

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

    struct Iter
    {
    private:
        size_t ctrlchunk_idx;
        ctrlmask_t present_mask;
        HashTbl<Key, Val> const &tbl;

        size_t idx()
        {
            return __builtin_ctz(present_mask) + ctrlchunk_idx * CtrlChunk::NR_BYTES;
        }

    public:
        Iter(HashTbl<Key, Val> const &tbl)
            : tbl(tbl)
        {
        }

        Iter &begin()
        {
            ctrlchunk_idx = 0;
            if (tbl.max_nr_entries == 0) {
                present_mask = 0;
            } else {
                present_mask = tbl.ctrlchunks_buf()->present_mask();
                find_next_present();
            }
            return *this;
        }

        Iter &end()
        {
            ctrlchunk_idx = tbl.max_nr_entries / CtrlChunk::NR_BYTES;
            return *this;
        }

        bool find_next_present()
        {
            if (*this == tbl.end()) {
                return false;
            }
            while (!present_mask) {
                ctrlchunk_idx++;
                if (*this == tbl.end()) {
                    return false;
                }
                present_mask = tbl.ctrlchunks_buf()[ctrlchunk_idx].present_mask();
            }
            return true;
        }

        Iter &operator++()
        {
            if (!present_mask) return *this; // we reached the end
            ctrlmask_t keep_mask = ~(1u << (ctrlmask_t)__builtin_ctz(present_mask));
            present_mask &= keep_mask;
            find_next_present(); // move cursor to next present
            return *this;
        }

        std::pair<Key const &, Val &> operator*()
        {
            Entry *e = tbl.entries_buf() + idx();
            return std::pair<Key const &, Val &>(e->key, e->val);
        }

        bool operator==(Iter const &other)
        {
            return ctrlchunk_idx == other.ctrlchunk_idx;
        }

        bool operator!=(Iter const &other)
        {
            return !(*this == other);
        }

        /**
         * Unsafe -- we are doing a pointer read of the entry, make sure you 
         * don't call the destructor for one of these 
         */
        void read(Entry *dst)
        {
            memcpy(dst, tbl.entries_buf() + idx(), sizeof(Entry));
        }
    };

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

    template <int COUNT> void prefetch_entries(Entry *e, ctrlmask_t mask)
    {
        for (int i = 0; i < COUNT; ++i) {
            // We don't want to branch, in the case that mask does not have
            // `COUNT` high-bits so we just always set the first bit high.
            mask |= 1 << 31;
            // if (!mask) break; // Naive could be better, but the above emits
            //                   // less instructions, so we'll go for that.
            int offset = __builtin_ctz(mask);
            __builtin_prefetch(e + offset, 0, 0);
            mask &= ~(1 << offset);
        }
    }

public:
    HashTbl()
        : buf(nullptr)
        , max_nr_entries(0)
        , nr_used(0)
    {
        PATH_AA = 0;
        PATH_AB = 0;
        PATH_B = 0;
        PATH_C = 0;
    }

    static Self with_capacity(size_t capacity)
    {
        auto self = Self();
        // alignup
        self.max_nr_entries = alignup(capacity, CtrlChunk::NR_BYTES);
        if (posix_memalign((void **)&self.buf, BUF_ALIGNMENT, self.buf_size())) {
            throw std::runtime_error("OOM");
        }
        memset(self.buf, CtrlChunk::CTRL_EMPTY, self.max_nr_entries);
        return self;
    }

    ~HashTbl()
    {
        if (buf) {
            for (auto kv : *this) {
                const_cast<Key &>(kv.first).~Key();
                kv.second.~Val();
            }
            free(buf);
            buf = nullptr;
        }
    }

    HashTbl(HashTbl &)
    {
        throw std::runtime_error("copy ctor not implemented");
    }

    HashTbl &operator=(HashTbl &other)
    {
        throw std::runtime_error("copy assignment not implemented");
    }

    HashTbl(HashTbl &&)
    {
        throw std::runtime_error("move ctor not implemented");
    }

    HashTbl &operator=(HashTbl &&rhs)
    {
        std::swap(*this, rhs);
        return *this;
    }

    size_t ctrlchunk_buf_size() const
    {
        // We need to make this assumption for our calculation to make sense --
        // that the byte directly after our ctrl chunks is ctrlchunk_t-aligned.
        // alignof(ctrlchunk_t) == sizeof(ctrlchunk_t)
        size_t padding_sz =
            alignof(ctrlchunk_t) > alignof(Entry) ? 0 : alignof(Entry) - alignof(ctrlchunk_t);
        return sizeof(ctrlchunk_t) * max_nr_entries + padding_sz;
    }

    size_t buf_size() const
    {
        return ctrlchunk_buf_size() + sizeof(Entry) * max_nr_entries;
    }

    Iter begin() const
    {
        return Iter(*this).begin();
    }

    Iter end() const
    {
        // The end is actually just an index that we compare against. No point
        // constructing a whole iterator
        return Iter(*this).end();
    }

    void grow()
    {
        auto newtbl =
            Self::with_capacity(max_nr_entries ? max_nr_entries * 4 : CtrlChunk::NR_BYTES * 4);
        for (auto it = begin(); it != end(); ++it) {
            // This is probably a C++ anti-pattern, but I come from Rust-land
            // Where it is also an anti-pattern, but ermm.... whatever?
            //
            // We want to move the `Entry` out of the old thing, but we don't
            // need to do a copy-and-swap move-assignment, so we just do a
            // memcpy and then a move ctor for both Key and Val.
            //
            // TODO: I will be shocked if there is a genuine need for this mess.
            // Fix this at some point.
            using ManuallyDropEntry =
                typename std::aligned_storage<sizeof(Entry), alignof(Entry)>::type;
            ManuallyDropEntry e_mu;
            it.read((Entry *)&e_mu);
            Entry &e = reinterpret_cast<Entry &>(static_cast<ManuallyDropEntry &>(e_mu));
            // TODO: we can save a bit of time by not rehashing
            newtbl.insert(std::move(e.key), std::move(e.val));
        }
        free(buf);
        buf = newtbl.buf;
        newtbl.buf = nullptr;
        max_nr_entries = newtbl.max_nr_entries;
        nr_used = newtbl.nr_used;
    }

    CtrlChunk *ctrlchunks_buf() const
    {
        return (CtrlChunk *)buf;
    }

    Entry *entries_buf() const
    {
        return (Entry *)(buf + ctrlchunk_buf_size());
    }

    void insert_unchecked(size_t idx, Entry e)
    {
        throw std::runtime_error("nye");
    }

    // Use a 0.75 load factor -- should be decent
    bool needs_to_grow() const
    {
        return nr_used >= max_nr_entries / 4 * 3;
    }

    uint64_t PATH_AA;
    uint64_t PATH_AB;
    uint64_t PATH_B;
    uint64_t PATH_C;

    /**
     * Get the slot where we can insert something with the provided `key`. 
     * 
     * # Returns
     * - `true` if the slot is empty
     * - `false` if it is occupied
     */
    bool get_slot(size_t h, Key const &key, Entry *&slot, char *&ctrl_slot)
    {
        if (needs_to_grow()) grow();

        Entry *entries = entries_buf();
        CtrlChunk *ctrlchunks = ctrlchunks_buf();

        // Just memoize some stuff for readability mostly
        size_t entry_idx = h % max_nr_entries;
        size_t ctrlchunk_idx = entry_idx / CtrlChunk::NR_BYTES;
        size_t ctrlbyte_offset = entry_idx % CtrlChunk::NR_BYTES;
        size_t aligned_entry_idx = ctrlchunk_idx * CtrlChunk::NR_BYTES;

        CtrlChunk ctrlchunk = *(ctrlchunks + ctrlchunk_idx);

        ctrlmask_t keep_mask = std::numeric_limits<ctrlmask_t>::max() << ctrlbyte_offset;
        ctrlmask_t hit_mask = simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), h7(h)) &
                              keep_mask;
        ctrlmask_t empty_mask =
            simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), CtrlChunk::CTRL_EMPTY) & keep_mask;

        auto empty_mask_tz = CtrlChunk::mask_ctz(empty_mask);
        auto hit_mask_tz = CtrlChunk::mask_ctz(hit_mask);
        while (true) {
            // Path distribution for 8,388,608 (2^23) randint insertions.
            // We do on average 1.04 loops... so probably best to just consider
            // the whole thing as one block of code.
            // PATH_AA = 8427816 = %
            // PATH_AB = 8126464 = %
            // PATH_B  = 262144  = %
            // PATH_C  = 39559   = %
            if (hit_mask_tz < empty_mask_tz) {
                PATH_AA++;
                // We have some kind of hit that we need to check is a complete hit
                ctrlbyte_offset = hit_mask_tz;
                size_t i = aligned_entry_idx + ctrlbyte_offset;
                Entry *entry = entries + i;
                if (cmp_keys(h, key, entry->hash, entry->key)) {
                    PATH_AB++;
                    slot = entry;
                    ctrl_slot = (char *)ctrlchunks + i;
                    return false;
                }
                hit_mask &= ~((ctrlmask_t)1 << hit_mask_tz);
                hit_mask_tz = CtrlChunk::mask_ctz(hit_mask);
            } else if (empty_mask) {
                PATH_B++;
                // If we have no matches, but there is an empty slot, we just
                // put it there
                ctrlbyte_offset = empty_mask_tz;
                size_t i = aligned_entry_idx + ctrlbyte_offset;
                slot = entries + i;
                ctrl_slot = (char *)ctrlchunks + i;
                return true;
            } else {
                PATH_C++;
                // If we have no matches and there is no empty slot, we must
                // continue probing in subsequent chunks
                aligned_entry_idx = (aligned_entry_idx + CtrlChunk::NR_BYTES) % max_nr_entries;
                ctrlchunk_idx = aligned_entry_idx / CtrlChunk::NR_BYTES;
                ctrlchunk = ctrlchunks[ctrlchunk_idx];
                hit_mask = simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), h7(h));
                empty_mask =
                    simd<ctrlchunk_t>::movemask_eq(ctrlchunk.as_simd(), CtrlChunk::CTRL_EMPTY);
                hit_mask_tz = CtrlChunk::mask_ctz(hit_mask);
                empty_mask_tz = CtrlChunk::mask_ctz(empty_mask);
            }
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
        bool empty = get_slot(h, key, slot, ctrl_slot);
        if (!empty) {
            slot->val = std::move(val);
        } else {
            new (slot) Entry(h, std::move(key), std::move(val));
            *ctrl_slot = h7(h);
        }
        nr_used++;
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

    void remove(Key const &key)
    {
        size_t h = is_hashable<Key>::hash(key);
        Entry *slot;
        char *ctrl_slot;
        // empty, no need to mark deleted
        if (get_slot(h, key, slot, ctrl_slot)) return;
        *ctrl_slot = CtrlChunk::CTRL_DEL;
        slot->key.~Key();
        slot->val.~Val();
    }
};
