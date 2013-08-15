#include <iostream>
#include <cassert>

#include <cocaine/framework/common.hpp>

#include <json/json.h>

#include "variant.hpp"
#include "traits.hpp"

using namespace cocaine;

struct print_visitor :
    public boost::static_visitor<>
{
    print_visitor(unsigned int indent) :
        m_indent(indent)
    {
        // pass
    }

    void
    operator()(const variant_t::null_t&) const {
        std::cout << "null";
    }

    void
    operator()(const variant_t::bool_t& v) const {
        std::cout << (v ? "True" : "False");
    }

    void
    operator()(const variant_t::int_t& v) const {
        std::cout << v;
    }

    void
    operator()(const variant_t::double_t& v) const {
        std::cout << v;
    }

    void
    operator()(const variant_t::string_t& v) const {
        std::cout << "'" << v << "'";
    }

    void
    operator()(const variant_t::array_t& v) const {
        std::cout << "[" << std::endl;

        bool print_coma = false;
        for(size_t i = 0; i < v.size(); ++i) {
            if (print_coma) {
                std::cout << "," << std::endl;
            }
            std::cout << std::string(2 * (m_indent + 1), ' ');
            v[i].apply(print_visitor(m_indent + 1));
            print_coma = true;
        }

        std::cout << std::endl << std::string(2 * m_indent, ' ') << "]";
    }

    void
    operator()(const variant_t::object_t& v) const {
        std::cout << "{" << std::endl;

        bool print_coma = false;
        for(auto it = v.begin(); it != v.end(); ++it) {
            if (print_coma) {
                std::cout << "," << std::endl;
            }
            std::cout << std::string(2 * (m_indent + 1), ' ') << "'" << it->first << "': ";
            it->second.apply(print_visitor(m_indent + 1));
            print_coma = true;
        }

        std::cout << std::endl << std::string(2 * m_indent, ' ') << "}";
    }

private:
    unsigned int m_indent;
};

void
test_msgpack() {
    variant_t d1 = variant_t::object_t();

    auto& obj = d1.as_object();
    obj["key1"] = 5;
    obj["key2"] = "privet";

    auto& key3 = obj["key3"].as_object();
    key3["nkey1"] = false;
    key3["nkey2"] = 1.0;
    key3["nkey3"] = std::make_tuple(1, 3.0, std::string("testec"));

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(buffer);
    cocaine::io::type_traits<variant_t>::pack(packer, d1);

    std::cout << "Original:" << std::endl;
    d1.apply(print_visitor(0));
    std::cout << std::endl;

    std::cout << "Packaged: " << std::string(buffer.data(), buffer.size()) << std::endl;

    variant_t d2 = cocaine::framework::unpack<variant_t>(buffer.data(), buffer.size());

    std::cout << "Unpackaged:" << std::endl;
    d2.apply(print_visitor(0));
    std::cout << std::endl;

    assert(d1 == d2);
}

const unsigned int MAX_DEPTH = 26;
const size_t BASE_SIZE = 120;

void
fill_variant(variant_t& dest, unsigned int depth) {
    int r = 0;

    if (depth < MAX_DEPTH / 5) {
        r = 4 + rand() % 2;
    } else if (depth > MAX_DEPTH) {
        r = rand() % 4;
    } else {
        r = rand() % 6;
    }

    if (r == 0) {
        dest = true;
    } else if (r == 1) {
        dest = 1337;
    } else if (r == 2) {
        dest = 64813.101;
    } else if (r == 3) {
        std::string s;
        size_t size = rand() % (10 * BASE_SIZE);
        s.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            s.push_back(char(30 + rand() % 30));
        }
        dest = std::move(s);
    } else if (r == 4) {
        std::vector<variant_t> v;
        size_t size = rand() % (BASE_SIZE / 10);
        v.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            variant_t d;
            fill_variant(d, depth + 1);
            v.emplace_back(std::move(d));
        }
        dest = std::move(v);
    } else if (r == 5) {
        variant_t::object_t obj;

        for (size_t i = 0; i < size_t(rand() % (BASE_SIZE / 10)); ++i) {
            std::string key;
            size_t size = rand() % BASE_SIZE;
            key.reserve(size);
            for (size_t i = 0; i < size; ++i) {
                key.push_back(char(30 + rand() % 30));
            }

            variant_t d;
            fill_variant(d, depth + 1);

            obj[key] = std::move(d);
        }

        dest = std::move(obj);
    }
}

struct variant_walker :
    public boost::static_visitor<>
{
    variant_walker() :
        m_nulls(0),
        m_bools(0),
        m_ints(0),
        m_doubles(0),
        m_strings(0),
        m_arrays(0),
        m_objects(0)
    {
        // pass
    }

    void
    operator()(const variant_t::null_t&) {
        ++m_nulls;
    }

    void
    operator()(const variant_t::bool_t&) {
        ++m_bools;
    }

    void
    operator()(const variant_t::int_t&) {
        ++m_ints;
    }

    void
    operator()(const variant_t::double_t&) {
        ++m_doubles;
    }

    void
    operator()(const variant_t::string_t&) {
        ++m_strings;
    }

    void
    operator()(const variant_t::array_t& v) {
        ++m_arrays;

        for (auto it = v.begin(); it != v.end(); ++it) {
            it->apply(*this);
        }
    }

    void
    operator()(const variant_t::object_t& v) {
        ++m_objects;

        for (auto it = v.begin(); it != v.end(); ++it) {
            it->second.apply(*this);
        }
    }

public:
    size_t m_nulls;
    size_t m_bools;
    size_t m_ints;
    size_t m_doubles;
    size_t m_strings;
    size_t m_arrays;
    size_t m_objects;
};

void
test_variant_performance() {
    srand(1337);

    variant_t d;

    std::cout << "Start variant perfomance test" << std::endl;

    fill_variant(d, 0);

    std::cout << "Variant object has been filled" << std::endl;

    time_t now = time(0);

    for (int i = 0; i < 10; ++i) {
        variant_walker w;
        d.apply(w);
    }

    std::cout << "Walk time: " << (time(0) - now) << std::endl;

    variant_walker w;

    d.apply(w);

    std::cout << "STAT: "
              << w.m_nulls << ", "
              << w.m_bools << ", "
              << w.m_ints << ", "
              << w.m_doubles << ", "
              << w.m_strings << ", "
              << w.m_arrays << ", "
              << w.m_objects << std::endl;
}

void
fill_json(Json::Value& dest, unsigned int depth) {
    int r = 0;

    if (depth < MAX_DEPTH / 5) {
        r = 4 + rand() % 2;
    } else if (depth > MAX_DEPTH) {
        r = rand() % 4;
    } else {
        r = rand() % 6;
    }

    if (r == 0) {
        dest = true;
    } else if (r == 1) {
        dest = 1337;
    } else if (r == 2) {
        dest = 64813.101;
    } else if (r == 3) {
        std::string s;
        size_t size = rand() % (10 * BASE_SIZE);
        s.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            s.push_back(char(30 + rand() % 30));
        }
        dest = std::move(s);
    } else if (r == 4) {
        size_t size = rand() % (BASE_SIZE / 10);
        dest.resize(size);
        for (size_t i = 0; i < size; ++i) {
            Json::Value d;
            fill_json(d, depth + 1);
            dest[(unsigned int)i] = std::move(d);
        }
    } else if (r == 5) {
        for (size_t i = 0; i < size_t(rand() % (BASE_SIZE / 10)); ++i) {
            std::string key;
            size_t size = rand() % BASE_SIZE;
            key.reserve(size);
            for (size_t i = 0; i < size; ++i) {
                key.push_back(char(30 + rand() % 30));
            }

            Json::Value d;
            fill_json(d, depth + 1);

            dest[key] = std::move(d);
        }
    }
}

struct json_walker :
    public boost::static_visitor<>
{
    json_walker() :
        m_nulls(0),
        m_bools(0),
        m_ints(0),
        m_doubles(0),
        m_strings(0),
        m_arrays(0),
        m_objects(0)
    {
        // pass
    }

    void
    walk(const Json::Value& source) {
        switch(source.type()) {
        case Json::objectValue: {
            ++m_objects;

            const Json::Value::Members keys(source.getMemberNames());

            for(auto it = keys.begin(); it != keys.end(); ++it) {
                walk(source[*it]);
            }
        } break;

        case Json::arrayValue: {
            ++m_arrays;

            for(auto it = source.begin(); it != source.end(); ++it) {
                walk(*it);
            }
        } break;

        case Json::booleanValue: {
            ++m_bools;
        } break;

        case Json::stringValue: {
            ++m_strings;
        } break;

        case Json::realValue: {
            ++m_doubles;
        } break;

        case Json::intValue: {
            ++m_ints;
        } break;

        case Json::uintValue: {
            ++m_ints;
        } break;

        case Json::nullValue: {
            ++m_nulls;
        }}
    }

public:
    size_t m_nulls;
    size_t m_bools;
    size_t m_ints;
    size_t m_doubles;
    size_t m_strings;
    size_t m_arrays;
    size_t m_objects;
};

void
test_json_performance() {
    srand(1337);

    Json::Value d;

    std::cout << "Start json perfomance test" << std::endl;

    fill_json(d, 0);

    std::cout << "Json object has been filled" << std::endl;

    time_t now = time(0);

    for (int i = 0; i < 10; ++i) {
        json_walker w;
        w.walk(d);
    }

    std::cout << "Walk time: " << (time(0) - now) << std::endl;

    json_walker w;

    w.walk(d);

    std::cout << "STAT: "
              << w.m_nulls << ", "
              << w.m_bools << ", "
              << w.m_ints << ", "
              << w.m_doubles << ", "
              << w.m_strings << ", "
              << w.m_arrays << ", "
              << w.m_objects << std::endl;
}


enum myen {
    case1,
    case2,
    case3
};

int
main() {
//#define PERFORM_PERFORMANCE_TEST
#ifdef PERFORM_PERFORMANCE_TEST
//    for (size_t i = 0; i < 500; ++i) {
//        test_variant_performance();
//        //test_json_performance();
//    }

    //test_variant_performance();
    test_json_performance();

    return 0;
#endif // PERFORM_PERFORMANCE_TEST

    {
        variant_t d1;
        assert(d1.is_null());
        assert(!d1.convertible_to<int>());
    }

    {
        variant_t d1 = false;
        assert(d1.is_bool());
        assert(!d1.as_bool());
        assert(d1.to<bool>() == false);

        assert(d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
    }

    {
        variant_t d1 = -1;
        auto d2 = d1;

        assert(d1.is_int());
        assert(d1.as_int() == -1);
        assert(d1 == -1);
        assert(d1 != -1.0);
        assert(d2.is_int());
        assert(d2 == -1);
        assert(d1.to<double>() == -1.0);
        assert(d1.to<char>() == -1);

        assert(!d1.convertible_to<bool>());
        assert(d1.convertible_to<int>());
        assert(d1.convertible_to<double>());
    }

    {
        variant_t d1 = case2;

        assert(d1.is_int());
        assert(d1.to<myen>() == case2);
    }

    {
        variant_t d1 = -1.0;

        assert(d1.is_double());
        assert(d1 == -1.0);
        assert(d1 != -1);
        assert(d1.to<double>() == -1.0);
        assert(d1.to<int>() == -1);
        assert(d1.to<char>() == -1);

        assert(!d1.convertible_to<bool>());
        assert(d1.convertible_to<int>());
        assert(d1.convertible_to<double>());
    }

    {
        variant_t d1 = std::string("test1");
        variant_t d2 = "test2";

        assert(d1.is_string() && d1.as_string() == "test1");
        assert(d2.is_string());
        assert(d2.as_string() == "test2");
        assert(d1.to<std::string>() == "test1");
        assert(d1.to<const char*>() == std::string("test1"));

        assert(!d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
        assert(!d1.convertible_to<double>());
        assert(d1.convertible_to<std::string>());
        assert(d1.convertible_to<const char*>());
    }

    {
        variant_t d1 = std::vector<int>(5, 33);

        assert(d1.is_array());
        assert(d1.as_array().size() == 5);
        assert(d1.as_array()[0].as_int() == 33);
        assert(d1.as_array()[2] == 33);
        assert(d1.to<std::vector<int>>().size() == 5);
        assert(d1.to<std::vector<int>>()[4] == 33);
        assert(std::get<3>(d1.to<std::tuple<int, int, int, int, int>>()) == 33);

        assert(!d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
        assert(!d1.convertible_to<double>());
        assert(!d1.convertible_to<std::string>());
        assert(!d1.convertible_to<const char*>());
        assert(d1.convertible_to<std::vector<int>>());
        assert(d1.convertible_to<std::vector<double>>());
        assert((d1.convertible_to<std::tuple<double, double, double, double, double>>()));
        assert(!(d1.convertible_to<std::tuple<double, double, double, double>>()));
    }

    {
        int arr[4] = {1, 2, 3, 4};
        variant_t d1 = arr;

        assert(d1.is_array());
        assert(d1.as_array().size() == 4);
        assert(d1.as_array()[0].as_int() == 1);
        assert(d1.as_array()[2] == 3);
        assert(d1.to<std::vector<int>>().size() == 4);
        assert(d1.to<std::vector<int>>()[3] == 4);
        assert(std::get<3>(d1.to<std::tuple<int, int, int, int>>()) == 4);
    }

    {
        variant_t d1 = std::tuple<int, std::string, std::string, double>(17, "test", "yo!", 1.0);

        assert(d1.is_array());
        assert(d1.as_array().size() == 4);
        assert(d1.as_array()[0].as_int() == 17);
        assert(d1.as_array()[1].as_string() == "test");
        assert(d1.as_array()[2].as_string() == "yo!");
        assert(d1.as_array()[3].as_double() == 1);

        assert(d1.as_array()[3].to<char>() == 1);

        assert(d1.as_array()[0] == 17);
        assert(d1.as_array()[1] == "test");
        assert(d1.as_array()[2] == "yo!");
        assert(d1.as_array()[3] == 1.0);
        assert(d1.as_array()[3] != 1);

        assert(std::get<0>(d1.to<std::tuple<double, const char*, std::string, int>>()) == 17);
        assert(std::get<1>(d1.to<std::tuple<double, const char*, std::string, int>>()) == std::string("test"));
        assert(std::get<2>(d1.to<std::tuple<double, const char*, std::string, int>>()) == "yo!");
        assert(std::get<3>(d1.to<std::tuple<double, const char*, std::string, int>>()) == 1);
    }

    {
        variant_t d1;

        auto& obj = d1.as_object();
        obj["key1"] = 5;
        obj["key2"] = "privet";

        auto& key3 = obj["key3"].as_object();
        key3["nkey1"] = false;
        key3["nkey2"] = 1.0;

        assert(d1.is_object());
        assert(obj.size() == 3);
        assert(obj["key1"] == 5);
        assert(obj["key2"] == "privet");
        assert(obj["key3"].is_object());
        assert(key3["nkey1"] == false);
        assert(key3["nkey2"] == 1.0);

        assert(key3.at("nkey1") == false);
        assert(key3.at("nkey1", 56) == false);
        assert(key3.at("nkey5", 56) == 56);

        const auto& const_key3 = obj["key3"].as_object();
        assert(const_key3["nkey1"] == false);
        assert(const_key3.at("nkey1", 56) == false);
        assert(const_key3.at("nkey5", 56) == 56);

        bool has_key = true;
        try {
            const_key3["nkey24"];
        } catch (...) {
            has_key = false;
        }
        assert(!has_key);

        assert(!d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
        assert(!d1.convertible_to<double>());
        assert(!d1.convertible_to<std::string>());
        assert(!d1.convertible_to<const char*>());
        assert(!d1.convertible_to<std::vector<int>>());
        assert(!d1.convertible_to<std::vector<double>>());
        assert(!(d1.convertible_to<std::tuple<double, double, double, double, double>>()));
    }

    {
        std::map<std::string, std::string> m;
        m["key1"] = "val1";
        m["key2"] = "val2";

        variant_t d1 = m;

        assert(d1.is_object());
        assert(d1.as_object().size() == 2);
        assert(d1.as_object()["key1"] == "val1");
        assert(d1.as_object()["key2"] == "val2");
        d1.as_object()["key2"] = "moo";
        assert(d1.as_object()["key2"] == "moo");
        assert((d1.to<std::map<std::string, const char*>>()["key1"]) == std::string("val1"));
        assert((d1.to<std::map<std::string, const char*>>()["key2"]) == std::string("moo"));

        assert(!d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
        assert(!d1.convertible_to<double>());
        assert(!d1.convertible_to<std::string>());
        assert(!d1.convertible_to<const char*>());
        assert(!d1.convertible_to<std::vector<int>>());
        assert(!d1.convertible_to<std::vector<double>>());
        assert((d1.convertible_to<std::map<std::string, const char*>>()));
    }

    test_msgpack();

    return 0;
}
