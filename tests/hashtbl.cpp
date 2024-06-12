#include <hashmap.hpp>
#include <ihashmap.hpp>
#include <unordered_map>
#include <cassert>

#define RUNTEST(fn)                                                    \
    ({                                                                 \
        std::cout << "\033[1;34m  start:\033[0m " << #fn << std::endl; \
        auto f = fn;                                                   \
        f();                                                           \
        std::cout << "\033[1;32msuccess:\033[0m " << #fn << std::endl; \
        0;                                                             \
    })

#define assert_eq(aexpr, bexpr)                         \
    ({                                                  \
        auto a = aexpr;                                 \
        auto b = bexpr;                                 \
        if (a != b) {                                   \
            std::cout << a << " != " << b << std::endl; \
            assert(a == b);                             \
        }                                               \
    })

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

template <template <typename, typename> typename OracleMap,
          template <typename, typename> typename TestMap>
struct test_suite
{
    static void test_uint64_inserts_persist()
    {
        TestMap<uint64_t, uint64_t> testmap;
        OracleMap<uint64_t, uint64_t> oraclemap;
        for (size_t i = 0; i < (1 << 12); i += 2) {
            IMap<TestMap, uint64_t, uint64_t>::insert(testmap, i, i);
            IMap<OracleMap, uint64_t, uint64_t>::insert(oraclemap, i, i);
        }
        for (size_t i = 0; i < (1 << 12); ++i) {
            if (i % 2 == 0) {
                size_t testv = IMap<TestMap, uint64_t, uint64_t>::get(testmap, i);
                size_t oraclev = IMap<OracleMap, uint64_t, uint64_t>::get(oraclemap, i);
                assert(testv == oraclev);
            } else {
            }
        }
    }

    static void test_uint64_marks_entries_contained()
    {
        TestMap<uint64_t, uint64_t> testmap;
        OracleMap<uint64_t, uint64_t> oraclemap;
        for (size_t i = 0; i < (1 << 12); i += 2) {
            IMap<TestMap, uint64_t, uint64_t>::insert(testmap, i, i);
            IMap<OracleMap, uint64_t, uint64_t>::insert(oraclemap, i, i);
        }
        for (size_t i = 0; i < (1 << 12); ++i) {
            bool testv = IMap<TestMap, uint64_t, uint64_t>::contains(testmap, i);
            bool oraclev = IMap<OracleMap, uint64_t, uint64_t>::contains(oraclemap, i);
            assert(testv == oraclev);
        }
    }

    static void test_int_overrides_old_val()
    {
        TestMap<int, int> testmap;
        IMap<TestMap, int, int>::insert(testmap, 5, 4);
        IMap<TestMap, int, int>::insert(testmap, 5, 42);
        assert_eq((IMap<TestMap, int, int>::get(testmap, 5)), 42);
    }

    static void test_sequence_of_random_operations_against_oracle()
    {
#define INSERT 0
#define CONTAINS 1
#define CONTAINS_GET 2
#define REMOVE 3
        // #define CLEAR 4
        TestMap<int, int> testmap;
        OracleMap<int, int> oraclemap;
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int> dis(0, std::numeric_limits<int>::max());
        int maxv = 1 << 19;
        for (size_t it = 0; it < (1 << 23); ++it) {
            if (it % 100000 == 0) {
                for (int n = 0; n < 10; ++n)
                    std::cout << "\b\b\b\b\b";
                std::cout << "tested " << it << " operations against oracle..." << std::flush;
            }
            int fn = dis(gen) % REMOVE;
            int k, v;
            switch (fn) {
            case INSERT: {
                k = dis(gen) % maxv;
                v = dis(gen) % maxv;
                IMap<TestMap, int, int>::insert(testmap, k, v);
                IMap<OracleMap, int, int>::insert(oraclemap, k, v);
                break;
            }
            case CONTAINS: {
                k = dis(gen) % maxv;
                bool testv = IMap<TestMap, int, int>::contains(testmap, k);
                bool oraclev = IMap<OracleMap, int, int>::contains(oraclemap, k);
                assert(testv == oraclev);
                break;
            }
            case CONTAINS_GET: {
                k = dis(gen) % maxv;
                bool testcontains = IMap<TestMap, int, int>::contains(testmap, k);
                bool oraclecontains = IMap<OracleMap, int, int>::contains(oraclemap, k);
                assert(testcontains == oraclecontains);
                if (!testcontains) continue;
                int testv = IMap<TestMap, int, int>::get(testmap, k);
                int oraclev = IMap<OracleMap, int, int>::get(oraclemap, k);
                if (testv != oraclev) {
                    for (auto kv : testmap) {
                        std::cout << kv.first << " : " << kv.second << std::endl;
                    }
                }
                assert_eq(testv, oraclev);
                break;
            }
            case REMOVE: {
                k = dis(gen) % maxv;
                v = dis(gen) % maxv;
                IMap<TestMap, int, int>::remove(testmap, k);
                IMap<OracleMap, int, int>::remove(oraclemap, k);
                break;
            }
            default:
                throw std::runtime_error("unreachable");
            }
        }
        std::cout << std::endl;
    }

    static void test_insert_randoms_doesnt_segfault() {
        size_t nr_insertions = 1 << 23;
        TestMap<size_t, size_t> map;
        for (size_t i = 0; i < nr_insertions; ++i) {
            size_t k = MAP_TEST_DATA[i % MAP_TEST_DATA.size()];
            IMap<TestMap, size_t, size_t>::insert(map, k, 0);
        }
        std::cout << "PATH_AA = " << map.PATH_AA << std::endl;
        std::cout << "PATH_AB = " << map.PATH_AB << std::endl;
        std::cout << "PATH_B = " << map.PATH_B << std::endl;
        std::cout << "PATH_C = " << map.PATH_C << std::endl;
    }
};

int main()
{
    using tests = test_suite<default_std_unordered_map_t, HashTbl>;
    RUNTEST(tests::test_uint64_inserts_persist);
    RUNTEST(tests::test_uint64_marks_entries_contained);
    RUNTEST(tests::test_int_overrides_old_val);
    RUNTEST(tests::test_insert_randoms_doesnt_segfault);
    RUNTEST(tests::test_sequence_of_random_operations_against_oracle);
    return 0;
}