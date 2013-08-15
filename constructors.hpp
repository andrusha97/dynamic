#ifndef COCAINE_VARIANT_CONSTRUCTORS_HPP
#define COCAINE_VARIANT_CONSTRUCTORS_HPP

#include "variant.hpp"

#include <tuple>
#include <unordered_map>

namespace cocaine {

template<>
struct variant_constructor<variant_t::null_t, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const variant_t::null_t& from, variant_t::value_t& to) {
        to = from;
    }
};

template<>
struct variant_constructor<bool, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(bool from, variant_t::value_t& to) {
        to = variant_t::bool_t(from);
    }
};

template<class From>
struct variant_constructor<From, typename std::enable_if<std::is_integral<From>::value>::type> {
    static const bool enable = true;

    static
    inline
    void
    convert(From from, variant_t::value_t& to) {
        to = variant_t::int_t(from);
    }
};

template<class From>
struct variant_constructor<From, typename std::enable_if<std::is_enum<From>::value>::type> {
    static const bool enable = true;

    static
    inline
    void
    convert(const From& from, variant_t::value_t& to) {
        to = variant_t::int_t(from);
    }
};

template<class From>
struct variant_constructor<From, typename std::enable_if<std::is_floating_point<From>::value>::type> {
    static const bool enable = true;

    static
    inline
    void
    convert(From from, variant_t::value_t& to) {
        to = variant_t::double_t(from);
    }
};

template<size_t N>
struct variant_constructor<char[N], void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const char *from, variant_t::value_t& to) {
        to = variant_t::string_t();
        boost::get<variant_t::string_t>(to).assign(from, N - 1);
    }
};

template<>
struct variant_constructor<std::string, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const std::string& from, variant_t::value_t& to) {
        to = from;
    }

    static
    inline
    void
    convert(std::string&& from, variant_t::value_t& to) {
        to = variant_t::string_t();
        boost::get<variant_t::string_t>(to) = std::move(from);
    }
};

template<>
struct variant_constructor<std::vector<variant_t>, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const std::vector<variant_t>& from, variant_t::value_t& to) {
        to = from;
    }

    static
    inline
    void
    convert(std::vector<variant_t>&& from, variant_t::value_t& to) {
        to = variant_t::array_t();
        boost::get<variant_t::array_t>(to) = std::move(from);
    }
};

template<class T>
struct variant_constructor<std::vector<T>, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const std::vector<T>& from, variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(from.size());
        for (size_t i = 0; i < from.size(); ++i) {
            arr.emplace_back(from[i]);
        }
    }

    static
    inline
    void
    convert(std::vector<T>&& from, variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(from.size());
        for (size_t i = 0; i < from.size(); ++i) {
            arr.emplace_back(std::move(from[i]));
        }
    }
};

template<class T, size_t N>
struct variant_constructor<T[N], void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const T (&from)[N], variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            arr.emplace_back(from[i]);
        }
    }

    static
    inline
    void
    convert(T (&&from)[N], variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            arr.emplace_back(std::move(from[i]));
        }
    }
};

template<class... Args>
struct variant_constructor<std::tuple<Args...>, void> {
    static const bool enable = true;

    template<size_t N, size_t I, class... Args2>
    struct copy_tuple_to_vector {
        static
        inline
        void
        convert(const std::tuple<Args2...>& from, variant_t::array_t& to) {
            to.emplace_back(std::get<I - 1>(from));
            copy_tuple_to_vector<N, I + 1, Args2...>::convert(from, to);
        }
    };

    template<size_t N, class... Args2>
    struct copy_tuple_to_vector<N, N, Args2...> {
        static
        inline
        void
        convert(const std::tuple<Args2...>& from, variant_t::array_t& to) {
            to.emplace_back(std::get<N - 1>(from));
        }
    };

    template<class... Args2>
    struct copy_tuple_to_vector<0, 1, Args2...> {
        static
        inline
        void
        convert(const std::tuple<Args2...>& from, variant_t::array_t& to) {
            // pass
        }
    };

    template<size_t N, size_t I, class... Args2>
    struct move_tuple_to_vector {
        static
        inline
        void
        convert(std::tuple<Args2...>& from, variant_t::array_t& to) {
            to.emplace_back(std::move(std::get<I - 1>(from)));
            move_tuple_to_vector<N, I + 1, Args2...>::convert(from, to);
        }
    };

    template<size_t N, class... Args2>
    struct move_tuple_to_vector<N, N, Args2...> {
        static
        inline
        void
        convert(std::tuple<Args2...>& from, variant_t::array_t& to) {
            to.emplace_back(std::move(std::get<N - 1>(from)));
        }
    };

    template<class... Args2>
    struct move_tuple_to_vector<0, 1, Args2...> {
        static
        inline
        void
        convert(std::tuple<Args2...>& from, variant_t::array_t& to) {
            // pass
        }
    };

    static
    inline
    void
    convert(const std::tuple<Args...>& from, variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(sizeof...(Args));
        copy_tuple_to_vector<sizeof...(Args), 1, Args...>::convert(from, arr);
    }

    static
    inline
    void
    convert(std::tuple<Args...>&& from, variant_t::value_t& to) {
        to = variant_t::array_t();
        variant_t::array_t& arr = boost::get<variant_t::array_t>(to);
        arr.reserve(sizeof...(Args));
        std::tuple<Args...> from_ = std::move(from);
        move_tuple_to_vector<sizeof...(Args), 1, Args...>::convert(from_, arr);
    }
};

template<class T>
struct variant_constructor<
    T,
    typename std::enable_if<std::is_convertible<T, variant_t::object_t>::value>::type
> {
    static const bool enable = true;

    static
    inline
    void
    convert(const T& from, variant_t::value_t& to) {
        to = variant_t::object_t(from);
    }

    static
    inline
    void
    convert(T&& from, variant_t::value_t& to) {
        to = variant_t::object_t();
        boost::get<variant_t::object_t>(to) = std::move(from);
    }
};

template<class T>
struct variant_constructor<std::map<std::string, T>, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const std::map<std::string, T>& from, variant_t::value_t& to) {
        to = variant_t::object_t();
        variant_t::object_t& obj = boost::get<variant_t::object_t>(to);
        for (auto it = from.begin(); it != from.end(); ++it) {
            obj.insert(variant_t::object_t::value_type(it->first, it->second));
        }
    }

    static
    inline
    void
    convert(std::map<std::string, T>&& from, variant_t::value_t& to) {
        to = variant_t::object_t();
        variant_t::object_t& obj = boost::get<variant_t::object_t>(to);
        for (auto it = from.begin(); it != from.end(); ++it) {
            obj.insert(variant_t::object_t::value_type(it->first, std::move(it->second)));
        }
    }
};

template<class T>
struct variant_constructor<std::unordered_map<std::string, T>, void> {
    static const bool enable = true;

    static
    inline
    void
    convert(const std::unordered_map<std::string, T>& from, variant_t::value_t& to) {
        to = variant_t::object_t();
        variant_t::object_t& obj = boost::get<variant_t::object_t>(to);
        for (auto it = from.begin(); it != from.end(); ++it) {
            obj.insert(it->first, it->second);
        }
    }

    static
    inline
    void
    convert(std::unordered_map<std::string, T>&& from, variant_t::value_t& to) {
        to = variant_t::object_t();
        variant_t::object_t& obj = boost::get<variant_t::object_t>(to);
        for (auto it = from.begin(); it != from.end(); ++it) {
            obj.insert(it->first, std::move(it->second));
        }
    }
};

} // namespace cocaine

#endif // COCAINE_VARIANT_CONSTRUCTORS_HPP
