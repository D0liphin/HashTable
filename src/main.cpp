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

template<typename T>
struct Wrapper
{
    T val;
    Wrapper(T v) : val(v) {} 
};

int main()
{
    auto shopping_list = HashTbl<std::string, int>::with_capacity(100);
    shopping_list.insert("apple", 5);
    int apple = *shopping_list.get("apple");
    std::cout << "shopping_list.get(\"apple\") = " << apple << std::endl;

    return 0;
}
