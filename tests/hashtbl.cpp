#include <hashmap.hpp>
#include <ihashmap.hpp>
#include <unordered_map>
#include <cassert>

#define RUNTEST(fn)                                                    \
    ({                                                                 \
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
        int maxv = 1 << 9;
        for (size_t _ = 0; _ < (1 << 12); ++_) {
            int fn = dis(gen) % REMOVE;
            int k, v;
            switch (fn) {
            case INSERT: {
                k = dis(gen) % maxv;
                v = dis(gen) % maxv;
                std::cout << "INSERT(" << k << ", " << v << ")" << std::endl;
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
                    std::cout << "k = " << k << std::endl;
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
    }
};

int main()
{
    using tests = test_suite<HashTbl, default_std_unordered_map_t>;
    RUNTEST(tests::test_uint64_inserts_persist);
    RUNTEST(tests::test_uint64_marks_entries_contained);
    RUNTEST(tests::test_sequence_of_random_operations_against_oracle);
    return 0;
}