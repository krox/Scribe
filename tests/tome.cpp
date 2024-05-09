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
}

TEST_CASE("reading a tome from json", "[tome]")
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

    auto tome = Tome::read_json_string(j, schema);
    REQUIRE(tome.is_dict());
    REQUIRE(tome["foo"].is_dict());
    REQUIRE(tome["foo"]["bar"].is_integer());
}