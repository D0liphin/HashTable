#include <ihashmap.hpp>
#include <hashmap.hpp>
#include <benchmark/benchmark.h>
#include <iostream>

template <size_t SZ> struct Garbage
{
    uint8_t _[SZ];

    Garbage()
    {
    }
};

static std::vector<size_t> MAP_TEST_DATA;

void setup_test_data()
{
    size_t const sz = 1 << 18;
    MAP_TEST_DATA.reserve(sz);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, std::numeric_limits<size_t>::max());
    for (size_t i = 0; i < sz; ++i) {
        MAP_TEST_DATA.push_back(dis(gen));
    }
}

void teardown_test_data()
{
    MAP_TEST_DATA = {};
}

static auto _ = []() {
    setup_test_data();
    return 0;
}();

template <template <typename, typename> typename Map> struct MapBenchmarks
{
    static void BM_insert_in_order(benchmark::State &state)
    {
        size_t nr_insertions = state.range(0);
        for (auto _ : state) {
            Map<size_t, size_t> map;
            for (size_t i = 0; i < nr_insertions; ++i) {
                IMap<Map, size_t, size_t>::insert(map, i, 0);
            }
        }
    }

    static void BM_insert_randoms(benchmark::State &state)
    {
        size_t nr_insertions = state.range(0);
        for (auto _ : state) {
            Map<size_t, size_t> map;
            for (size_t i = 0; i < nr_insertions; ++i) {
                size_t k = MAP_TEST_DATA[i % MAP_TEST_DATA.size()];
                IMap<Map, size_t, size_t>::insert(map, k, 0);
            }
        }
    }

    static void BM_insert_2update_randoms(benchmark::State &state)
    {
        size_t nr_insertions = state.range(0);
        for (auto _ : state) {
            Map<size_t, size_t> map;
            for (size_t i = 0; i < nr_insertions * 3; ++i) {
                size_t k = MAP_TEST_DATA[i % nr_insertions];
                IMap<Map, size_t, size_t>::insert(map, k, 0);
            }
        }
    }

    static void BM_insert_in_order_xl_vals(benchmark::State &state)
    {
        size_t nr_insertions = state.range(0);
        for (auto _ : state) {
            Map<size_t, Garbage<512>> map;
            for (size_t i = 0; i < nr_insertions; ++i) {
                IMap<Map, size_t, Garbage<512>>::insert(map, i, Garbage<512>());
            }
        }
    }

    static void BM_insert_randoms_xl_vals(benchmark::State &state)
    {
        size_t nr_insertions = state.range(0);
        for (auto _ : state) {
            Map<size_t, Garbage<512>> map;
            for (size_t i = 0; i < nr_insertions; ++i) {
                size_t k = MAP_TEST_DATA[i % MAP_TEST_DATA.size()];
                IMap<Map, size_t, Garbage<512>>::insert(map, k, Garbage<512>());
            }
        }
    }
};

BENCHMARK(MapBenchmarks<default_std_unordered_map_t>::BM_insert_in_order)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<default_std_unordered_map_t>::BM_insert_randoms)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<default_std_unordered_map_t>::BM_insert_2update_randoms)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<default_std_unordered_map_t>::BM_insert_in_order_xl_vals)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<default_std_unordered_map_t>::BM_insert_randoms_xl_vals)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<HashTbl>::BM_insert_in_order)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<HashTbl>::BM_insert_randoms)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<HashTbl>::BM_insert_2update_randoms)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<HashTbl>::BM_insert_in_order_xl_vals)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<HashTbl>::BM_insert_randoms_xl_vals)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<Table>::BM_insert_in_order)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<Table>::BM_insert_randoms)->Range(8, 8 << 13);
BENCHMARK(MapBenchmarks<Table>::BM_insert_2update_randoms)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<Table>::BM_insert_in_order_xl_vals)->Range(8, 8 << 13);
// BENCHMARK(MapBenchmarks<Table>::BM_insert_randoms_xl_vals)->Range(8, 8 << 13);

BENCHMARK_MAIN();