#include <iostream>
#include <cassert>

#include <cocaine/framework/common.hpp>

#include <json/json.h>

#include "dynamic.hpp"
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
    operator()(const dynamic_t::null_t&) const {
        std::cout << "null";
    }

    void
    operator()(const dynamic_t::bool_t& v) const {
        std::cout << (v ? "True" : "False");
    }

    void
    operator()(const dynamic_t::int_t& v) const {
        std::cout << v;
    }

    void
    operator()(const dynamic_t::double_t& v) const {
        std::cout << v;
    }

    void
    operator()(const dynamic_t::string_t& v) const {
        std::cout << "'" << v << "'";
    }

    void
    operator()(const dynamic_t::array_t& v) const {
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
    operator()(const dynamic_t::object_t& v) const {
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
    dynamic_t d1 = dynamic_t::object_t();

    d1["key1"] = 5;
    d1["key2"] = "privet";
    d1["key3"] = dynamic_t::object_t();
    d1["key3"]["nkey1"] = false;
    d1["key3"]["nkey2"] = 1.0;
    d1["key3"]["nkey3"] = std::make_tuple(1, 3.0, std::string("testec"));

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(buffer);
    cocaine::io::type_traits<dynamic_t>::pack(packer, d1);

    std::cout << "Original:" << std::endl;
    d1.apply(print_visitor(0));
    std::cout << std::endl;

    std::cout << "Packaged: " << std::string(buffer.data(), buffer.size()) << std::endl;

    dynamic_t d2 = cocaine::framework::unpack<dynamic_t>(buffer.data(), buffer.size());

    std::cout << "Unpackaged:" << std::endl;
    d2.apply(print_visitor(0));
    std::cout << std::endl;

    assert(d1 == d2);
}

const unsigned int MAX_DEPTH = 20;
const size_t BASE_SIZE = 100;

void
fill_dynamic(dynamic_t& dest, unsigned int depth) {
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
        std::vector<dynamic_t> v;
        size_t size = rand() % (BASE_SIZE / 10);
        v.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            dynamic_t d;
            fill_dynamic(d, depth + 1);
            v.emplace_back(std::move(d));
        }
        dest = std::move(v);
    } else if (r == 5) {
        dest = dynamic_t::object_t();

        for (size_t i = 0; i < size_t(rand() % (BASE_SIZE / 10)); ++i) {
            std::string key;
            size_t size = rand() % BASE_SIZE;
            key.reserve(size);
            for (size_t i = 0; i < size; ++i) {
                key.push_back(char(30 + rand() % 30));
            }

            dynamic_t d;
            fill_dynamic(d, depth + 1);

            dest[key] = std::move(d);
        }
    }
}

struct dynamic_walker :
    public boost::static_visitor<>
{
    dynamic_walker() :
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
    operator()(const dynamic_t::null_t&) {
        ++m_nulls;
    }

    void
    operator()(const dynamic_t::bool_t&) {
        ++m_bools;
    }

    void
    operator()(const dynamic_t::int_t&) {
        ++m_ints;
    }

    void
    operator()(const dynamic_t::double_t&) {
        ++m_doubles;
    }

    void
    operator()(const dynamic_t::string_t&) {
        ++m_strings;
    }

    void
    operator()(const dynamic_t::array_t& v) {
        ++m_arrays;

        for (auto it = v.begin(); it != v.end(); ++it) {
            it->apply(*this);
        }
    }

    void
    operator()(const dynamic_t::object_t& v) {
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
test_dynamic_performance() {
    srand(1337);

    dynamic_t d;

//    std::cout << "Start dynamic perfomance test" << std::endl;

    fill_dynamic(d, 0);

//    std::cout << "Dynamic object has been filled" << std::endl;

    dynamic_walker w;

    d.apply(w);

//    std::cout << "STAT: "
//              << w.m_nulls << ", "
//              << w.m_bools << ", "
//              << w.m_ints << ", "
//              << w.m_doubles << ", "
//              << w.m_strings << ", "
//              << w.m_arrays << ", "
//              << w.m_objects << std::endl;
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

//    std::cout << "Start json perfomance test" << std::endl;

    fill_json(d, 0);

//    std::cout << "Json object has been filled" << std::endl;

    json_walker w;

    w.walk(d);

//    std::cout << "STAT: "
//              << w.m_nulls << ", "
//              << w.m_bools << ", "
//              << w.m_ints << ", "
//              << w.m_doubles << ", "
//              << w.m_strings << ", "
//              << w.m_arrays << ", "
//              << w.m_objects << std::endl;
}


enum myen {
    case1,
    case2,
    case3
};

int
main() {
#ifdef PERFORM_PERFORMANCE_TEST
    for (size_t i = 0; i < 500; ++i) {
        //test_dynamic_performance();
        test_json_performance();
    }

    //test_dynamic_performance();
    //test_json_performance();

    return 0;
#endif // PERFORM_PERFORMANCE_TEST

    {
        dynamic_t d1;
        assert(d1.is_null());
        assert(!d1.convertible_to<int>());
    }

    {
        dynamic_t d1 = false;
        assert(d1.is_bool());
        assert(!d1.as_bool());
        assert(d1.to<bool>() == false);

        assert(d1.convertible_to<bool>());
        assert(!d1.convertible_to<int>());
    }

    {
        dynamic_t d1 = -1;
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
        dynamic_t d1 = case2;

        assert(d1.is_int());
        assert(d1.to<myen>() == case2);
    }

    {
        dynamic_t d1 = -1.0;

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
        dynamic_t d1 = std::string("test1");
        dynamic_t d2 = "test2";

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
        dynamic_t d1 = std::vector<int>(5, 33);

        assert(d1.is_array());
        assert(d1.size() == 5);
        assert(d1[0].as_int() == 33);
        assert(d1[2] == 33);
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
        dynamic_t d1 = arr;

        assert(d1.is_array());
        assert(d1.size() == 4);
        assert(d1[0].as_int() == 1);
        assert(d1[2] == 3);
        assert(d1.to<std::vector<int>>().size() == 4);
        assert(d1.to<std::vector<int>>()[3] == 4);
        assert(std::get<3>(d1.to<std::tuple<int, int, int, int>>()) == 4);
    }

    {
        dynamic_t d1 = std::tuple<int, std::string, std::string, double>(17, "test", "yo!", 1.0);

        assert(d1.is_array());
        assert(d1.size() == 4);
        assert(d1[0].as_int() == 17);
        assert(d1[1].as_string() == "test");
        assert(d1[2].as_string() == "yo!");
        assert(d1[3].as_double() == 1);

        assert(d1[3].to<char>() == 1);

        assert(d1[0] == 17);
        assert(d1[1] == "test");
        assert(d1[2] == "yo!");
        assert(d1[3] == 1.0);
        assert(d1[3] != 1);

        assert(std::get<0>(d1.to<std::tuple<double, const char*, std::string, int>>()) == 17);
        assert(std::get<1>(d1.to<std::tuple<double, const char*, std::string, int>>()) == std::string("test"));
        assert(std::get<2>(d1.to<std::tuple<double, const char*, std::string, int>>()) == "yo!");
        assert(std::get<3>(d1.to<std::tuple<double, const char*, std::string, int>>()) == 1);
    }

    {
        dynamic_t d1 = dynamic_t::object_t();

        d1["key1"] = 5;
        d1["key2"] = "privet";
        d1["key3"] = dynamic_t::object_t();
        d1["key3"]["nkey1"] = false;
        d1["key3"]["nkey2"] = 1.0;

        assert(d1.is_object());
        assert(d1.size() == 3);
        assert(d1["key1"] == 5);
        assert(d1["key2"] == "privet");
        assert(d1["key3"].is_object());
        assert(d1["key3"]["nkey1"] == false);
        assert(d1["key3"]["nkey2"] == 1.0);
        assert(d1.has_key("key3"));
        assert(!d1.has_key("key4"));
        assert(d1.at_or("key4", 50.0).to<int>() == 50);
        assert(d1.at_or("key1", 50.0).to<int>() == 5);

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
        dynamic_t d1 = m;

        assert(d1.is_object());
        assert(d1.size() == 2);
        assert(d1["key1"] == "val1");
        assert(d1["key2"] == "val2");
        d1["key2"] = "moo";
        assert(d1["key2"] == "moo");
        assert(d1.has_key("key2"));
        assert(!d1.has_key("key4"));
        assert(d1.at_or("key4", 50.0).to<int>() == 50);
        assert(d1.at_or("key1", 50.0).to<std::string>() == "val1");
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
