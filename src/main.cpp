#include <iostream>
#include <string>

#include "hashmap.hpp"

class Logger
{
public:
    // Default Constructor
    Logger()
    {
        std::cout << "Logger()\n";
    }

    // Copy Constructor
    Logger(const Logger &other)
    {
        std::cout << "Logger(const Logger &)\n";
    }

    // Move Constructor
    Logger(Logger &&other) noexcept
    {
        std::cout << "Logger(Logger &&other)\n";
    }

    // Copy Assignment Operator
    Logger &operator=(const Logger &other)
    {
        if (this != &other) {
            std::cout << "Logger &operator=Logger(const Logger &)\n";
        }
        return *this;
    }

    // Move Assignment Operator
    Logger &operator=(Logger &&other) noexcept
    {
        if (this != &other) {
            std::cout << "Logger &operator=(Logger &&)\n";
        }
        return *this;
    }

    // Destructor
    ~Logger()
    {
        std::cout << "~Logger()\n";
    }
};

template <typename T> struct Wrapper
{
    T val;
    Wrapper(T v)
        : val(v)
    {
    }
};

int main()
{
    auto shopping_list = HashTbl<std::string, int>::with_capacity(16);
    shopping_list.insert("apple", 5);
    shopping_list.insert("banana", 2);
    shopping_list.insert("milk", 1);
    for (auto kv : shopping_list) {
        std::cout << kv.first << ": " << kv.second << std::endl;
    }
    return 0;
}

// #include <cassert

// // Test Initialization
// void hashtbl_initialization() {
//     HashTbl<std::string, uint32_t> new_table;
//     assert(new_table.get("anything") == nullptr);  // Expecting null as the table is empty.
// }

// // Test Basic Insert and Get
// void hashtbl_basic_insert_get() {
//     HashTbl<std::string, uint32_t> basic_test;
//     basic_test.insert("item1", 100);
//     assert(*basic_test.get("item1") == 100);
// }

// // Test Inserting Duplicate Keys
// void hashtbl_duplicate_keys() {
//     HashTbl<std::string, uint32_t> duplicate_key_test;
//     duplicate_key_test.insert("item1", 100);
//     duplicate_key_test.insert("item1", 200);  // Should overwrite the previous value
//     assert(*duplicate_key_test.get("item1") == 200);
// }

// // Test Insert and Get with Multiple Items
// void hashtbl_multiple_items() {
//     HashTbl<std::string, uint32_t> multiple_items;
//     multiple_items.insert("item1", 100);
//     multiple_items.insert("item2", 200);
//     multiple_items.insert("item3", 300);
//     assert(*multiple_items.get("item1") == 100);
//     assert(*multiple_items.get("item2") == 200);
//     assert(*multiple_items.get("item3") == 300);
// }

// // Test Get Nonexistent Key
// void hashtbl_get_nonexistent_key() {
//     HashTbl<std::string, uint32_t> nonexistent_key_test;
//     nonexistent_key_test.insert("item1", 100);
//     assert(nonexistent_key_test.get("item2") == nullptr);
// }

// // Test Case Sensitivity
// void hashtbl_case_sensitivity() {
//     HashTbl<std::string, uint32_t> case_sensitivity_test;
//     case_sensitivity_test.insert("Item1", 100);
//     assert(case_sensitivity_test.get("item1") == nullptr); // Assuming case sensitivity
//     assert(*case_sensitivity_test.get("Item1") == 100);
// }

// // Test Inserting Non-String Keys (if applicable)
// void hashtbl_int_keys() {
//     // This test depends on HashTbl supporting types other than std::string for keys
//     HashTbl<int, uint32_t> int_keys;
//     int_keys.insert(1, 100);
//     assert(*int_keys.get(1) == 100);
// }

// // Test Stress
// void hashtbl_stress_test() {
//     HashTbl<std::string, uint32_t> stress_test;
//     for (int i = 0; i < 10000; i++) {
//         stress_test.insert("item" + std::to_string(i), i);
//     }
//     for (int i = 0; i < 10000; i++) {
//         assert(*stress_test.get("item" + std::to_string(i)) == i);
//     }
// }

// int main() {
//     hashtbl_initialization();
//     hashtbl_basic_insert_get();
//     hashtbl_duplicate_keys();
//     hashtbl_multiple_items();
//     hashtbl_get_nonexistent_key();
//     // hashtbl_case_sensitivity();
//     // hashtbl_int_keys();
//     // hashtbl_stress_test();

//     std::cout << "All tests passed!" << std::endl;
//     return 0;
// }