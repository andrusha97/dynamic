#include "variant.hpp"

using namespace cocaine;

cocaine::variant_t&
detail::variant::object_t::at(const std::string& key, cocaine::variant_t& def) {
    auto it = find(key);
    if (it == end()) {
        return def;
    } else {
        return it->second;
    }
}

const cocaine::variant_t&
detail::variant::object_t::at(const std::string& key, const cocaine::variant_t& def) const {
    auto it = find(key);
    if (it == end()) {
        return def;
    } else {
        return it->second;
    }
}

const cocaine::variant_t&
detail::variant::object_t::operator[](const std::string& key) const {
    return at(key);
}

struct is_empty_visitor :
    public boost::static_visitor<bool>
{
    bool
    operator()(const variant_t::null_t&) const {
        return true;
    }

    bool
    operator()(const variant_t::bool_t&) const {
        return false;
    }

    bool
    operator()(const variant_t::int_t&) const {
        return false;
    }

    bool
    operator()(const variant_t::double_t&) const {
        return false;
    }

    bool
    operator()(const variant_t::string_t& v) const {
        return v.empty();
    }

    bool
    operator()(const variant_t::array_t& v) const {
        return v.empty();
    }

    bool
    operator()(const variant_t::object_t& v) const {
        return v.empty();
    }
};

struct move_visitor :
    public boost::static_visitor<>
{
    move_visitor(variant_t& dest) :
        m_dest(dest)
    {
        // pass
    }

    template<class T>
    void
    operator()(T& v) const {
        m_dest = std::move(v);
    }

private:
    variant_t& m_dest;
};

variant_t::variant_t() :
    m_value(null_t())
{
// pass
}

variant_t::variant_t(const variant_t& other) :
    m_value(other.m_value)
{
    // pass
}

variant_t::variant_t(variant_t&& other) :
    m_value(null_t())
{
    other.apply(move_visitor(*this));
}

variant_t&
variant_t::operator=(const variant_t& other) {
    m_value = other.m_value;
    return *this;
}

variant_t&
variant_t::operator=(variant_t&& other) {
    other.apply(move_visitor(*this));
    return *this;
}

bool
variant_t::operator==(const variant_t& other) const {
    return m_value == other.m_value;
}

bool
variant_t::operator!=(const variant_t& other) const {
    return !(m_value == other.m_value);
}

variant_t::bool_t
variant_t::as_bool() const {
    return get<bool_t>();
}

variant_t::int_t
variant_t::as_int() const {
    return get<int_t>();
}

variant_t::double_t
variant_t::as_double() const {
    return get<double_t>();
}

const variant_t::string_t&
variant_t::as_string() const {
    return get<string_t>();
}

const variant_t::array_t&
variant_t::as_array() const {
    return get<array_t>();
}

const variant_t::object_t&
variant_t::as_object() const {
    return get<object_t>();
}

variant_t::string_t&
variant_t::as_string() {
    if (is_null()) {
        m_value = string_t();
    }

    return get<string_t>();
}

variant_t::array_t&
variant_t::as_array() {
    if (is_null()) {
        m_value = array_t();
    }

    return get<array_t>();
}

variant_t::object_t&
variant_t::as_object() {
    if (is_null()) {
        m_value = object_t();
    }

    return get<object_t>();
}

bool
variant_t::is_null() const {
    return is<null_t>();
}

bool
variant_t::is_bool() const {
    return is<bool_t>();
}

bool
variant_t::is_int() const {
    return is<int_t>();
}

bool
variant_t::is_double() const {
    return is<double_t>();
}

bool
variant_t::is_string() const {
    return is<string_t>();
}

bool
variant_t::is_array() const {
    return is<array_t>();
}

bool
variant_t::is_object() const {
    return is<object_t>();
}
