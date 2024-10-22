#include "catch2/catch_test_macros.hpp"

#include "fmt/format.h"
#include "scribe/tome.h"

using scribe::Schema;
using scribe::Tome;

TEST_CASE("explicit-type constructors", "[tome]")
{
    SECTION("bool")
    {
        auto tome = Tome::boolean(true);
        REQUIRE_NOTHROW(tome.as<bool>() == true);
    }
    SECTION("string")
    {
        auto tome = Tome::string("hello");
        REQUIRE_NOTHROW(tome.as<std::string>() == "hello");
    }
    SECTION("int")
    {
        REQUIRE_NOTHROW(Tome::integer((int8_t)42).as<int8_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((int16_t)42).as<int16_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((int32_t)42).as<int32_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((int64_t)42).as<int64_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((uint8_t)42).as<uint8_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((uint16_t)42).as<uint16_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((uint32_t)42).as<uint32_t>() == 42);
        REQUIRE_NOTHROW(Tome::integer((uint64_t)42).as<uint64_t>() == 42);
    }
    SECTION("real")
    {
        REQUIRE_NOTHROW(Tome::real(3.14f).as<float>() == 3.14f);
        REQUIRE_NOTHROW(Tome::real(3.14).as<double>() == 3.14);
    }
    SECTION("complex")
    {
        std::complex<float> c(1.0f, 2.0f);
        REQUIRE_NOTHROW(Tome::complex(c).as<std::complex<float>>() == c);
        REQUIRE_NOTHROW(
            Tome::complex(1.0, 2.0).as<scribe::complex_float64_t>() ==
            scribe::complex_float64_t(1.0, 2.0));
    }
    SECTION("dict")
    {
        REQUIRE_NOTHROW(Tome().as_dict().empty());
        scribe::Tome::dict_type dict = {{"foo", 42}, {"bar", 3.14}};
        auto tome = Tome::dict(std::move(dict));
        assert(dict.empty()); // should be "moved-from"
        REQUIRE_NOTHROW(tome.as_dict().size() == 2);
        REQUIRE_NOTHROW(tome.as_dict()["foo"].as<int>() == 42);
        REQUIRE_NOTHROW(tome.as_dict()["bar"].as<double>() == 3.14);
        tome["baz"] = "hello";
        REQUIRE_NOTHROW(tome.as_dict().size() == 3);
        REQUIRE_NOTHROW(tome.as_dict()["baz"].as<std::string>() == "hello");
    }
    SECTION("standard array 1D")
    {
        std::vector<Tome> vec = {1, 2, 3};
        auto tome = Tome::array(vec);
        REQUIRE_NOTHROW(tome.as_array().size() == 3);
        REQUIRE_NOTHROW(tome.as_array()[0].as<int>() == 1);
        REQUIRE_NOTHROW(tome.as_array()[1].as<int>() == 2);
        REQUIRE_NOTHROW(tome.as_array()[2].as<int>() == 3);
        tome.push_back(4);
        REQUIRE_NOTHROW(tome.as_array().size() == 4);
        REQUIRE_NOTHROW(tome.as_array()[3].as<int>() == 4);
    }
    SECTION("standard array 2d")
    {
        auto tome = Tome::array_from_shape({2, 3});
        REQUIRE_NOTHROW(tome.as_array().shape().size() == 2);
        REQUIRE_NOTHROW(tome.as_array().shape()[0] == 2);
        REQUIRE_NOTHROW(tome.as_array().shape()[1] == 3);
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
                tome.as_array()(i, j) = i * 3 + j + 1;

        std::vector<Tome> data;
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
                data.push_back(i * 3 + j + 1);
        auto tome2 = Tome::array(data, {2, 3});

        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
            {
                REQUIRE_NOTHROW(tome.as_array()(i, j).as<int>() ==
                                i * 3 + j + 1);
                REQUIRE_NOTHROW(tome2.as_array()(i, j).as<int>() ==
                                i * 3 + j + 1);
            }
    }
    SECTION("standard array 2d from data") {}
}

TEST_CASE("libfmt support of tome", "[tome]") {}

TEST_CASE("scribe::Tome as generic type", "[tome]")
{
    SECTION("default constructor")
    {
        Tome tome;
        REQUIRE(tome.is_dict());
    }

    SECTION("boolean constructor")
    {
        scribe::Tome tome(true);
        REQUIRE(tome.is_boolean());
        REQUIRE(tome.get<bool>());
    }

    SECTION("integer constructor")
    {
        scribe::Tome tome(42);
        REQUIRE(tome.is_integer());
        REQUIRE(tome.get<int>() == 42);
    }

    SECTION("nested dicts")
    {
        Tome tome;
        tome["foo"]["bar"] = 42;
        REQUIRE(tome.is_dict());
        REQUIRE(tome["foo"].is_dict());
        REQUIRE(tome["foo"]["bar"].is_integer());
        REQUIRE(tome["foo"]["bar"].get<int>() == 42);
    }

    SECTION("numeric array")
    {
        Tome tome = Tome(std::vector<float>({1.0, 2.0, 3.0}));
        REQUIRE(tome.is_array());
        REQUIRE(tome.size() == 3);
        auto data = tome.get<std::vector<float>>();
        REQUIRE(data.size() == 3);
        REQUIRE(data[0] == 1.0);
        REQUIRE(data[1] == 2.0);
        REQUIRE(data[2] == 3.0);
    }
}

TEST_CASE("reading a tome from json", "[tome]")
{
    SECTION("basic example")
    {
        auto schema = Schema::from_json(R"(
    {
        "type": "dict",
        "items": [
            {
                "key": "foo",
                "type": "dict",
                "items": [
                    {
                        "key": "bar",
                        "type": "int32"
                    }
                ]
            }
            ]
    }
    )"_json);

        std::string j = R"(
    {
        "foo": {
            "bar": 42
        }
    }
    )";

        std::string j2 = R"(
    {
        "foo": {
            "bar": "42"
        }
    }
    )";
        Tome tome;
        read_json_string(tome, j, schema);
        REQUIRE(tome.is_dict());
        REQUIRE(tome["foo"].is_dict());
        REQUIRE(tome["foo"]["bar"].is_integer());

        REQUIRE_THROWS(read_json_string(tome, j2, schema));
    }

    SECTION("multi-dim array")
    {
        auto schema = Schema::from_json(R"(
        {
            "type": "array",
            "shape": [2, -1],
            "elements": {
                "type": "int32"
            }
        }
        )"_json);

        std::string j = R"(
        [
            [1, 2, 3],
            [4, 5, 6]
        ]
        )";

        std::string j2 = R"(
        [
            [1, 2, 3],
            [4, 5]
        ]
        )";

        Tome tome;
        read_json_string(tome, j, schema);
        REQUIRE(tome.is_array());
        REQUIRE(tome.shape().size() == 2);
        REQUIRE(tome.shape()[0] == 2);
        REQUIRE(tome.shape()[1] == 3);
        REQUIRE(tome.as_array()(0, 0).is_integer());
        REQUIRE(tome.as_array()(0, 0).get<int>() == 1);
        REQUIRE(tome.as_array()(1, 2).get<int>() == 6);

        REQUIRE_THROWS(read_json_string(tome, j2, schema));
    }

    SECTION("strings")
    {
        auto schema = Schema::from_json(R"(
        {
            "type": "dict",
            "items": [
                {
                    "key": "foo",
                    "type": "string",
                    "min_length": 2,
                    "max_length": 4
                }
            ]
        }
        )"_json);

        std::string j = R"({"foo": "abc"})";
        std::string j2 = R"({"foo": ""})";
        std::string j3 = R"({"foo": "abcdef"})";

        Tome tome;
        read_json_string(tome, j, schema);
        REQUIRE(tome.is_dict());
        REQUIRE(tome["foo"].is_string());
        REQUIRE(tome["foo"].get<std::string>() == "abc");

        REQUIRE_THROWS(read_json_string(tome, j2, schema));
        REQUIRE_THROWS(read_json_string(tome, j3, schema));
    }
}