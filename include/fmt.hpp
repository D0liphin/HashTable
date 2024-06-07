#pragma once

#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fmt {

/*
 * @brief Trait allowing a type to be formatted. For this trait to be valid, you
 * must have
 * @brief - `static const bool implements = true;`
 * @brief - a valid member function: `static std::ostream &fmt(std::ostream &, T
 * const &);`
 * @endcode
 * @tparam T The type that is to be formatted
 * @tparam ...Args optional additional arguments
 */
template <typename T, typename... Args> struct debug {
    static const bool implements = false;
};

/// @brief Marker to tell us that we want to format this type using the
/// `debug` formatter
/// @tparam T The type to format
template <typename T, typename... Args> struct Debug {
    using formatter = debug<T, Args...>;
    static_assert(formatter::implements, "Type does not implement debug");
    using Inner =
        typename std::remove_reference<typename std::remove_cv<T>::type>::type;

    Inner const &inner;

    Debug(Inner const &inner) : inner(inner) {}

    template <typename... NewArgs> Debug<Inner, NewArgs...> args() {
        return Debug<Inner, NewArgs...>(std::move(this->inner));
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    Debug<Inner, Args...> const &self) {
        return formatter::fmt(os, self.inner);
    }
};

template <typename T, typename... Args> Debug<T, Args...> dbg(T const &value) {
    return Debug<T, Args...>(value);
}

#define IMPL_DEBUG(T)                                                          \
    template <> struct debug<T> {                                              \
        static const bool implements = true;                                   \
                                                                               \
        static std::ostream &fmt(std::ostream &os, T const &self) {            \
            os << self;                                                        \
            return os;                                                         \
        }                                                                      \
    };

IMPL_DEBUG(signed char)
IMPL_DEBUG(unsigned char)
IMPL_DEBUG(short int)
IMPL_DEBUG(unsigned short int)
IMPL_DEBUG(int)
IMPL_DEBUG(unsigned int)
IMPL_DEBUG(long int)
IMPL_DEBUG(unsigned long int)
IMPL_DEBUG(long long int)
IMPL_DEBUG(unsigned long long int)

template <> struct debug<std::string> {
    static const bool implements = true;

    static std::ostream &fmt(std::ostream &os, std::string const &self) {
        return os << "\"" << self << "\"";
    }
};

template <typename Iterator, typename Item>
std::ostream &
fmtCollection(std::ostream &os, Iterator iter, std::string const &lBrace,
              std::function<void(std::ostream &, Item const &)> put,
              std::string const &delimiter, std::string const &rBrace,
              std::string const &empty) {
    if (iter.size() == 0) {
        return os << empty;
    }
    auto vec = std::vector<Item>();
    for (auto item : iter) {
        vec.push_back(item);
    }

    os << lBrace;
    auto end = --vec.end();
    for (auto it = vec.begin(); it != end; ++it) {
        put(os, *it);
        os << delimiter;
    }
    put(os, *end);
    return os << rBrace;
}

template <typename T> struct debug<std::vector<T>> {
    static const bool implements = true;
    static_assert(debug<T>::implements,
                  "std::vector<T> requires that T is debug for it to "
                  "derive debug");

    static std::ostream &fmt(std::ostream &os, std::vector<T> const &self) {
        return fmtCollection<std::vector<T>, T>(
            os, self, "{", [](std::ostream &os, T const &item) { os << item; },
            ", ", "}", "{}");
    }
};

template <typename K, typename V> struct debug<std::unordered_map<K, V>> {
    static const bool implements = true;

    static_assert(debug<K>::implements && debug<V>::implements,
                  "std::unordered_map<K, V> requires that both K and V are "
                  "`debug` for it to be `debug");

    static std::ostream &fmt(std::ostream &os,
                             std::unordered_map<K, V> const &self) {
        return fmtCollection<std::unordered_map<K, V>, std::pair<K, V>>(
            os, self, "{",
            [](std::ostream &os, auto const &item) {
                os << dbg(item.first) << ": " << dbg(item.second);
            },
            ", ", "}", "{}");
    }
};

struct Pretty {};

template <typename K, typename V>
struct debug<std::unordered_map<K, V>, Pretty> {
    static const bool implements = true;

    static_assert(debug<K>::implements && debug<V>::implements,
                  "std::unordered_map<K, V> requires that both K and V are "
                  "`debug` for it to be `debug");

    static std::ostream &fmt(std::ostream &os,
                             std::unordered_map<K, V> const &self) {
        return fmtCollection<std::unordered_map<K, V>, std::pair<K, V>>(
            os, self, "{\n",
            [](std::ostream &os, auto const &item) {
                os << "   " << dbg(item.first) << ": " << dbg(item.second);
            },
            ",\n", "\n}", "{}");
    }
};

} // namespace fmt