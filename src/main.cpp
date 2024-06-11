#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>

#include "ihashmap.hpp"
#include "hashmap.hpp"

template <typename T> class Logger
{
public:
    bool moved;
    std::string name;
    T val;

    Logger(std::string name, T val)
        : name(name)
        , val(val)
        , moved(false)
    {
        std::cout << name << "(" << val << ")" << std::endl;
    }

    Logger(const Logger &other)
    {
        name = other.name;
        val = other.val;
        moved = false;
        std::cout << name << "(const " << name << " &)" << std::endl;
    }

    // Move Constructor
    Logger(Logger &&other) noexcept
    {
        name = std::move(other.name);
        other.name = name + "[moved]";
        other.moved = true;
        val = std::move(other.val);
        moved = false;
        std::cout << "\t\t\t\t" << name << "(" << name << " &&)" << std::endl;
    }

    // Copy Assignment Operator
    Logger &operator=(const Logger &other)
    {
        if (this != &other) {
            ~Logger();
            new (this) Logger(other);
            std::cout << name << " &operator=" << name << "(const" << name << "&)" << std::endl;
        }
        return *this;
    }

    // Move Assignment Operator
    Logger &operator=(Logger &&other) noexcept
    {
        if (this != &other) {
            name = std::move(other.name);
            val = std::move(other.val);
            std::cout << name << " &operator=(" << name << " &&)" << std::endl;
        }
        return *this;
    }

    // Destructor
    ~Logger()
    {
        if (moved) std::cout << "\t\t\t\t";
        std::cout << "~" << name << "()" << std::endl;
        val.~T();
    }

    friend std::ostream &operator<<(std::ostream &os, Logger<T> const &self)
    {
        return os << "Logger(" << self.val << ")";
    }

    bool operator==(Logger<T> const &other) const
    {
        return val == other.val;
    }
};

template <typename T> struct is_hashable<Logger<T>>
{
    static constexpr bool value = is_hashable<T>::value;

    static size_t hash(Logger<T> const &self)
    {
        return is_hashable<T>::hash(self.val);
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

std::string bin(size_t n)
{
    std::string str;
    for (size_t i = 0; i < std::numeric_limits<size_t>::digits; ++i) {
        size_t mask = ~(std::numeric_limits<size_t>::max() >> 1) >> i;
        str.push_back((n & mask) ? '1' : '0');
    }
    return str;
}

int main()
{
    // for (size_t nr_insertions = 8; nr_insertions <= (8 << 8); nr_insertions <<= 1) {
    //     HashTbl<size_t, size_t> tbl;
    //     std::cout << "Doing " << nr_insertions << " insertions" << std::endl;
    //     for (size_t i = 0; i < nr_insertions; ++i) {
    //         tbl.insert(i, 0);
    //     }
    // }
    return 0;
}
