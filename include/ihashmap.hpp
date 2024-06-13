#pragma once

#include <hashmap.hpp>
#include <string>
#include <unordered_map>
#include <random>
#include <vector>
#include <stdexcept>
#include <table.h>

/**
 * It is also assumed that `Map` provides a `std::pair` iterator.
 */
template <template <typename, typename> typename Map, typename K, typename V> struct IMap
{
    static constexpr bool implements = false;

    /**
     * Insert a (key, value) pair into the map, or update the value at the
     * specified key
     */
    static void insert(Map<K, V> &, K, V)
    {
        throw std::runtime_error("unimplemented");
    }

    /**
     * Tests whether or not the map contains a (key, value) pair with the 
     * specified `key`.
     */
    static bool contains(Map<K, V> const &, K const &)
    {
        throw std::runtime_error("unimplemented");
    }

    /**
     * Get a reference to a value at a specific key, UB if the key does not
     * exist. 
     */
    static V &get(Map<K, V> &map, K const &k)
    {
        throw std::runtime_error("unimplemented");
    }

    /**
     * Remove a value from the map.
     */
    static void remove(Map<K, V> &, K const &)
    {
        throw std::runtime_error("unimplemented");
    }

    /**
     * Delete all owned elements from this hashtable.
     */
    static void clear(Map<K, V> &)
    {
        throw std::runtime_error("unimplemented");
    }
};

template <typename K, typename V>
using default_std_unordered_map_t =
    std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, std::allocator<std::pair<const K, V>>>;

template <typename K, typename V> struct IMap<default_std_unordered_map_t, K, V>
{
    using Map = default_std_unordered_map_t<K, V>;
    static constexpr bool implements = true;

    static void insert(Map &map, K k, V v)
    {
        map[k] = std::move(v);
    }

    static bool contains(Map const &map, K const &k)
    {
        return !!map.count(k);
    }

    static V &get(Map &map, K const &k)
    {
        return map[k];
    }

    static void remove(Map &map, K const &k)
    {
        map.erase(k);
    }

    static void clear(Map &map)
    {
        map.clear();
    }
};

template <typename K, typename V> struct IMap<HashTbl, K, V>
{
    using Map = HashTbl<K, V>;
    static constexpr bool implements = true;

    static void insert(Map &map, K k, V v)
    {
        map.insert(std::move(k), std::move(v));
    }

    static bool contains(Map &map, K const &k)
    {
        return map.get(k) != nullptr;
    }

    static V &get(Map &map, K const &k)
    {
        return *map.get(k);
    }

    static void remove(Map &map, K const &k)
    {
        map.remove(k);
    }

    static void clear(Map &map)
    {
        throw std::runtime_error("nyi");
    }
};

template <typename K, typename V> struct Table
{
    Table()
    {
        table_init(&inner, 0);
    }

    ~Table() 
    {
        table_free(&inner);
    }

    table inner;
};

template <typename K, typename V> struct IMap<Table, K, V>
{
    using Map = Table<K, V>;
    static constexpr bool implements = true;

    static void insert(Map &map, K k, V v)
    {
        table_insert(&map.inner, (_key_t)k, (val_t)v);
    }

    static bool contains(Map &map, K const &k)
    {
        return table_get(&map.inner, (_key_t const &)k) != nullptr;
    }

    static V &get(Map &map, K const &k)
    {
        return (V &)*table_get(&map.inner, (_key_t const &)k);
    }

    static void remove(Map &map, K const &k)
    {
        table_remove(&map.inner, (_key_t const &)k);
    }

    static void clear(Map &map)
    {
        throw std::runtime_error("nyi");
    }
};