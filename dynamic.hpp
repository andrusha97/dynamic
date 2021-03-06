#ifndef COCAINE_DYNAMIC_HPP
#define COCAINE_DYNAMIC_HPP

#include <boost/variant.hpp>

#include <string>
#include <vector>
#include <map>
#include <type_traits>

namespace cocaine {

class dynamic_t;

namespace detail { namespace dynamic {

    template<class ConstVisitor, class Result>
    struct const_visitor_applier :
        public boost::static_visitor<Result>
    {
        const_visitor_applier(ConstVisitor v) :
            m_const_visitor(v)
        {
            // pass
        }

        template<class T>
        Result
        operator()(T& v) const {
            return m_const_visitor(static_cast<const T&>(v));
        }

    private:
        ConstVisitor m_const_visitor;
    };

    template<class T>
    struct my_decay {
        typedef typename std::remove_reference<T>::type unref;
        typedef typename std::remove_cv<unref>::type type;
    };

    class object_t :
        public std::map<std::string, cocaine::dynamic_t>
    {
        typedef std::map<std::string, cocaine::dynamic_t>
                base_type;
    public:
        object_t() {
            // pass
        }

        template<class InputIt>
        object_t(InputIt first, InputIt last) :
            base_type(first, last)
        {
            // pass
        }

        object_t(const object_t& other) :
            base_type(other)
        {
            // pass
        }

        object_t(object_t&& other) :
            base_type(std::move(other))
        {
            // pass
        }

        object_t(std::initializer_list<value_type> init) :
            base_type(init)
        {
            // pass
        }

        object_t(const base_type& other) :
            base_type(other)
        {
            // pass
        }

        object_t(base_type&& other) :
            base_type(std::move(other))
        {
            // pass
        }

        object_t&
        operator=(const object_t& other) {
            base_type::operator=(other);
            return *this;
        }

        object_t&
        operator=(object_t&& other) {
            base_type::operator=(std::move(other));
            return *this;
        }

        using base_type::at;

        cocaine::dynamic_t&
        at(const std::string& key, cocaine::dynamic_t& def);

        const cocaine::dynamic_t&
        at(const std::string& key, const cocaine::dynamic_t& def) const;

        using base_type::operator[];

        const cocaine::dynamic_t&
        operator[](const std::string& key) const;
    };

}} // namespace detail::dynamic

template<class From, class = void>
struct dynamic_constructor {
    static const bool enable = false;
};

template<class To, class = void>
struct dynamic_converter {
    // pass
};

class dynamic_t {
public:
    struct null_t {
        bool
        operator==(const null_t&) const {
            return true;
        }
    };

    typedef bool
            bool_t;
    typedef int64_t
            int_t;
    typedef double
            double_t;
    typedef std::string
            string_t;
    typedef std::vector<dynamic_t>
            array_t;
    typedef detail::dynamic::object_t
            object_t;

    typedef boost::variant<null_t,
                           bool_t,
                           int_t,
                           double_t,
                           string_t,
                           array_t,
                           object_t>
            value_t;

public:
    dynamic_t();

    dynamic_t(const dynamic_t& other);

    dynamic_t(dynamic_t&& other);

    template<class T>
    dynamic_t(
        T&& from,
        typename std::enable_if<dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::enable>::type* = 0
    );

    dynamic_t&
    operator=(const dynamic_t& other);

    dynamic_t&
    operator=(dynamic_t&& other);

    template<class T>
    typename std::enable_if<dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::enable, dynamic_t&>::type
    operator=(T&& from);

    bool
    operator==(const dynamic_t& other) const;

    bool
    operator!=(const dynamic_t& other) const;

    template<class Visitor>
    typename Visitor::result_type
    apply(Visitor& visitor) {
        return boost::apply_visitor(visitor, m_value);
    }

    template<class Visitor>
    typename Visitor::result_type
    apply(const Visitor& visitor) {
        return boost::apply_visitor(visitor, m_value);
    }

    template<class Visitor>
    typename Visitor::result_type
    apply(const Visitor& visitor) const {
        return boost::apply_visitor(
            detail::dynamic::const_visitor_applier<const Visitor&, typename Visitor::result_type>(visitor),
            m_value
        );
    }

    template<class Visitor>
    typename Visitor::result_type
    apply(Visitor& visitor) const {
        return boost::apply_visitor(
            detail::dynamic::const_visitor_applier<Visitor&, typename Visitor::result_type>(visitor),
            m_value
        );
    }

    bool
    is_null() const;

    bool
    is_bool() const;

    bool
    is_int() const;

    bool
    is_double() const;

    bool
    is_string() const;

    bool
    is_array() const;

    bool
    is_object() const;

    bool_t
    as_bool() const;

    int_t
    as_int() const;

    double_t
    as_double() const;

    const string_t&
    as_string() const;

    const array_t&
    as_array() const;

    const object_t&
    as_object() const;

    string_t&
    as_string();

    array_t&
    as_array();

    object_t&
    as_object();

    // it returns bool, but is enabled only for T for which dynamic_converter::result_type is defined
    template<class T>
    typename std::conditional<
        true,
        bool,
        typename dynamic_converter<typename detail::dynamic::my_decay<T>::type>::result_type
    >::type
    convertible_to() const;

    template<class T>
    typename dynamic_converter<typename detail::dynamic::my_decay<T>::type>::result_type
    to() const;

private:
    template<class T>
    T&
    get() {
        return boost::get<T>(m_value);
    }

    template<class T>
    const T&
    get() const {
        return boost::get<T>(m_value);
    }

    template<class T>
    bool
    is() const {
        return static_cast<bool>(boost::get<T>(&m_value));
    }

private:
    // boost::apply_visitor takes non-constant reference to variable object
    mutable value_t m_value;
};

template<class T>
dynamic_t::dynamic_t(
    T&& from,
    typename std::enable_if<dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::enable>::type*
) : m_value(null_t())
{
    dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::convert(std::forward<T>(from), m_value);
}

template<class T>
typename std::enable_if<dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::enable, dynamic_t&>::type
dynamic_t::operator=(T&& from) {
    dynamic_constructor<typename detail::dynamic::my_decay<T>::type>::convert(std::forward<T>(from), m_value);
    return *this;
}

template<class T>
typename std::conditional<
    true,
    bool,
    typename dynamic_converter<typename detail::dynamic::my_decay<T>::type>::result_type
>::type
dynamic_t::convertible_to() const {
    return dynamic_converter<typename detail::dynamic::my_decay<T>::type>::convertible(*this);
}

template<class T>
typename dynamic_converter<typename detail::dynamic::my_decay<T>::type>::result_type
dynamic_t::to() const {
    return dynamic_converter<typename detail::dynamic::my_decay<T>::type>::convert(*this);
}

} // namespace cocaine

#include "constructors.hpp"
#include "converters.hpp"

#endif // COCAINE_DYNAMIC_HPP
