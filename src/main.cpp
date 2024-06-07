#include <iostream>
#include <string>

#include "hashmap.hpp"

int main()
{
    auto shopping_list = HashTbl<std::string, int>::with_capacity(100);
    shopping_list.insert("apple", 5);
    
    return 0;
}
