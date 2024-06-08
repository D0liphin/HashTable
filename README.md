# `HashTbl`

I always wanted to make a hash table. Here is one that is inspired by
Google's swiss table... The implementations are quite different though.
I hope to fill this README with a bunch of benchmarks soon.

## Usage

```cpp
HashTbl<std::string, uint32_t> shopping_list;
shopping_list.insert("pint o' milk", 2);
shopping_list.insert("effective modern c++", 1);
shopping_list.insert("flash boys", 1);

uint32_t *nr_pints_o_milk = shopping_list.get("pint o' milk");
```

