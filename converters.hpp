#ifndef COCAINE_VARIANT_CONVERTERS_HPP
#define COCAINE_VARIANT_CONVERTERS_HPP

#include "variant.hpp"

#include <tuple>
#include <unordered_map>

namespace cocaine {

template<>
struct variant_converter<variant_t, void> {
    typedef const variant_t& result_type;

    static
    const variant_t&
    convert(const variant_t& from) {
        return from;
    }

    static
    bool
    convertible(const variant_t&) {
        return true;
    }
};

template<>
struct variant_converter<bool, void> {
    typedef bool result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_bool();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_bool();
    }
};

template<class To>
struct variant_converter<To, typename std::enable_if<std::is_arithmetic<To>::value>::type> {
    typedef To result_type;

    static
    result_type
    convert(const variant_t& from) {
        if (from.is_int()) {
            return from.as_int();
        } else {
            return from.as_double();
        }
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_int() || from.is_double();
    }
};

template<class To>
struct variant_converter<To, typename std::enable_if<std::is_enum<To>::value>::type> {
    typedef To result_type;

    static
    result_type
    convert(const variant_t& from) {
        return static_cast<result_type>(from.as_int());
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_int();
    }
};

template<>
struct variant_converter<std::string, void> {
    typedef const std::string& result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_string();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_string();
    }
};

template<>
struct variant_converter<const char*, void> {
    typedef const char *result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_string().c_str();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_string();
    }
};

template<>
struct variant_converter<std::vector<variant_t>, void> {
    typedef const std::vector<variant_t>& result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_array();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_array();
    }
};

template<class T>
struct variant_converter<std::vector<T>, void> {
    typedef std::vector<T> result_type;

    static
    result_type
    convert(const variant_t& from) {
        std::vector<T> result;
        const variant_t::array_t& array = from.as_array();
        for (size_t i = 0; i < array.size(); ++i) {
            result.emplace_back(array[i].to<T>());
        }
        return result;
    }

    static
    bool
    convertible(const variant_t& from) {
        if (from.is_array()) {
            const variant_t::array_t& array = from.as_array();
            for (size_t i = 0; i < array.size(); ++i) {
                if (!array[i].convertible_to<T>()) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }
};

template<class... Args>
struct variant_converter<std::tuple<Args...>, void> {
    typedef std::tuple<Args...> result_type;

    static
    result_type
    convert(const variant_t& from) {
        if (sizeof...(Args) == from.as_array().size()) {
            if (sizeof...(Args) == 0) {
                return result_type();
            } else {
                return range_applier<sizeof...(Args) - 1>::conv(from.as_array());
            }
        } else {
            throw std::bad_cast();
        }
    }

    static
    bool
    convertible(const variant_t& from) {
        if (from.is_array() && sizeof...(Args) == from.as_array().size()) {
            if (sizeof...(Args) == 0) {
                return true;
            } else {
                return range_applier<sizeof...(Args) - 1>::is_conv(from.as_array());
            }
        } else {
            return false;
        }
    }

private:
    template<size_t... Idxs>
    struct range_applier;

    template<size_t First, size_t... Idxs>
    struct range_applier<First, Idxs...> :
        range_applier<First - 1, First, Idxs...>
    {
        static
        inline
        bool
        is_conv(const variant_t::array_t& from) {
            return from[First].convertible_to<typename std::tuple_element<First, result_type>::type>() &&
                   range_applier<First - 1, First, Idxs...>::is_conv(from);
        }
    };

    template<size_t... Idxs>
    struct range_applier<0, Idxs...> {
        static
        inline
        result_type
        conv(const variant_t::array_t& from) {
            return std::tuple<Args...>(
                from[0].to<typename std::tuple_element<0, result_type>::type>(),
                from[Idxs].to<typename std::tuple_element<Idxs, result_type>::type>()...
            );
        }

        static
        inline
        bool
        is_conv(const variant_t::array_t& from) {
            return from[0].convertible_to<typename std::tuple_element<0, result_type>::type>();
        }
    };
};

template<>
struct variant_converter<variant_t::object_t, void> {
    typedef const variant_t::object_t& result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_object();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_object();
    }
};

template<>
struct variant_converter<std::map<std::string, variant_t>, void> {
    typedef const std::map<std::string, variant_t>& result_type;

    static
    result_type
    convert(const variant_t& from) {
        return from.as_object();
    }

    static
    bool
    convertible(const variant_t& from) {
        return from.is_object();
    }
};

template<class T>
struct variant_converter<std::map<std::string, T>, void> {
    typedef std::map<std::string, T> result_type;

    static
    result_type
    convert(const variant_t& from) {
        result_type result;
        const variant_t::object_t& object = from.as_object();
        for (auto it = object.begin(); it != object.end(); ++it) {
            result.insert(typename result_type::value_type(it->first, it->second.to<T>()));
        }
        return result;
    }

    static
    bool
    convertible(const variant_t& from) {
        if (from.is_object()) {
            const variant_t::object_t& object = from.as_object();
            for (auto it = object.begin(); it != object.end(); ++it) {
                if (!it->second.convertible_to<T>()) {
                    return false;
                }

                return true;
            }
        }

        return false;
    }
};

template<class T>
struct variant_converter<std::unordered_map<std::string, T>, void> {
    typedef std::unordered_map<std::string, T> result_type;

    static
    result_type
    convert(const variant_t& from) {
        result_type result;
        const variant_t::object_t& object = from.as_object();
        for (auto it = object.begin(); it != object.end(); ++it) {
            result.insert(typename result_type::value_type(it->first, it->second.to<T>()));
        }
        return result;
    }

    static
    bool
    convertible(const variant_t& from) {
        if (from.is_object()) {
            const variant_t::object_t& object = from.as_object();
            for (auto it = object.begin(); it != object.end(); ++it) {
                if (!it->second.convertible_to<T>()) {
                    return false;
                }

                return true;
            }
        }

        return false;
    }
};

} // namespace cocaine

#endif // COCAINE_VARIANT_CONVERTERS_HPP
