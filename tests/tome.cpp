#include "catch2/catch_test_macros.hpp"

#include "fmt/format.h"
#include "scribe/tome.h"

using scribe::Schema;
using scribe::Tome;

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

    SECTION("explicit types")
    {
        auto tome = Tome::boolean(true);
        REQUIRE(tome.is_boolean());
        REQUIRE(tome.get<bool>());

        tome = Tome::integer(42);
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