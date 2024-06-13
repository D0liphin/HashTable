# `HashTbl`

I always wanted to make a hash table. Here is one that is inspired by
Google's swiss table... The implementations are quite different though.
Read about the high-level design [here](https://www.oliveriliffe.net/blog/hashtbl.html)
I hope to fill this README with a bunch of benchmarks soon.

## Usage

```cpp
HashTbl<std::string, uint32_t> shopping_list;
shopping_list.insert("pint o' milk", 2);
shopping_list.insert("effective modern c++", 1);
shopping_list.insert("flash boys", 1);

uint32_t *nr_pints_o_milk = shopping_list.get("pint o' milk");
```

## Benchmarks

I only benchmark for insertion on integers at the moment. In the future,
I will create benchmarks for other types, but there's not much point 
right now. The key and value don't need to be copy-constructable since
they are only ever moved. `remove()` destructs immediately.

Here are the current benchmarks, the src is 
`/benchmarksrc/benchmark.cpp`.

```plaintext
------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
std::unorderd_map::BM_insert_in_order/8                      220 ns          220 ns      3471544
std::unorderd_map::BM_insert_in_order/64                    2186 ns         2186 ns       320194
std::unorderd_map::BM_insert_in_order/512                  21333 ns        21332 ns        31668
std::unorderd_map::BM_insert_in_order/4096                181469 ns       181463 ns         3857
std::unorderd_map::BM_insert_in_order/8192                374371 ns       374347 ns         1873
std::unorderd_map::BM_insert_randoms/8                       236 ns          236 ns      3035124
std::unorderd_map::BM_insert_randoms/64                     2637 ns         2637 ns       281515
std::unorderd_map::BM_insert_randoms/512                   29090 ns        29084 ns        24538
std::unorderd_map::BM_insert_randoms/4096                 313133 ns       313103 ns         2228
std::unorderd_map::BM_insert_randoms/8192                 688833 ns       688831 ns         1015
std::unorderd_map::BM_insert_in_order_xl_vals/8              548 ns          547 ns      1115328
std::unorderd_map::BM_insert_in_order_xl_vals/64            5814 ns         5814 ns       115719
std::unorderd_map::BM_insert_in_order_xl_vals/512          50199 ns        50199 ns        10000
std::unorderd_map::BM_insert_in_order_xl_vals/4096        384678 ns       384646 ns         1793
std::unorderd_map::BM_insert_in_order_xl_vals/8192        880726 ns       880667 ns          749
std::unorderd_map::BM_insert_randoms_xl_vals/8               566 ns          564 ns      1139290
std::unorderd_map::BM_insert_randoms_xl_vals/64             5787 ns         5786 ns       104916
std::unorderd_map::BM_insert_randoms_xl_vals/512           49193 ns        49194 ns        11614
std::unorderd_map::BM_insert_randoms_xl_vals/4096         519260 ns       519256 ns         1238
std::unorderd_map::BM_insert_randoms_xl_vals/8192        1255661 ns      1255638 ns          590
HashTbl::BM_insert_in_order/8                                187 ns          187 ns      3627242
HashTbl::BM_insert_in_order/64                              1954 ns         1954 ns       357618
HashTbl::BM_insert_in_order/512                            11903 ns        11903 ns        57803
HashTbl::BM_insert_in_order/4096                          145636 ns       145632 ns         4537
HashTbl::BM_insert_in_order/8192                          208057 ns       208019 ns         3380
HashTbl::BM_insert_randoms/8                                 175 ns          175 ns      4063626
HashTbl::BM_insert_randoms/64                               1835 ns         1835 ns       378929
HashTbl::BM_insert_randoms/512                             12183 ns        12182 ns        57960
HashTbl::BM_insert_randoms/4096                           167404 ns       167384 ns         4086
HashTbl::BM_insert_randoms/8192                           265991 ns       265977 ns         2519
HashTbl::BM_insert_in_order_xl_vals/8                        842 ns          842 ns       820093
HashTbl::BM_insert_in_order_xl_vals/64                     11715 ns        11715 ns        57537
HashTbl::BM_insert_in_order_xl_vals/512                    78361 ns        78362 ns         8116
HashTbl::BM_insert_in_order_xl_vals/4096                  992662 ns       992600 ns          688
HashTbl::BM_insert_in_order_xl_vals/8192                 1438085 ns      1438060 ns          467
HashTbl::BM_insert_randoms_xl_vals/8                         889 ns          889 ns       793126
HashTbl::BM_insert_randoms_xl_vals/64                      12152 ns        12151 ns        56694
HashTbl::BM_insert_randoms_xl_vals/512                     82166 ns        82167 ns         8420
HashTbl::BM_insert_randoms_xl_vals/4096                  1181978 ns      1181971 ns          477
HashTbl::BM_insert_randoms_xl_vals/8192                  1818555 ns      1818338 ns          375
```

## Testing

I'm pretty confident this works in its current state. I just test it
against a bunch of random values and perform the same operations on
`std::unordered_map`. No flags for undefined-behaviour even after 
100,000,000 operations.
