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
        REQUIRE_NOTHROW(tome.rank() == 2);
        REQUIRE_NOTHROW(tome.size() == 6);
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

TEST_CASE("explicit type checking in tome", "[tome]")
{
    SECTION("integers")
    {
        auto tome = Tome::integer(42);
        REQUIRE(tome.is_integer());
        REQUIRE(tome.is_number());
        REQUIRE(!tome.is_real());
        REQUIRE(!tome.is_complex());
    }

    SECTION("arrays")
    {
        auto tome = Tome::array();
        REQUIRE(tome.is_array());
        REQUIRE(!tome.is_dict());
        REQUIRE(!tome.is_numeric_array());
        REQUIRE(tome.is<scribe::Tome::array_type>());

        auto tome2 = Tome::array_from_shape<double>({2, 3});
        REQUIRE(tome2.is_array());
        REQUIRE(tome2.is_numeric_array());

        auto tome3 = Tome::array(std::vector<int>({1, 2, 3}));
        REQUIRE(tome3.is_array());
        REQUIRE(tome3.is_numeric_array());
    }
}

TEST_CASE("libfmt support of tome", "[tome]")
{
    SECTION("bool")
    {
        auto tome = Tome::boolean(true);
        REQUIRE(fmt::format("{}", tome) == "true");
    }
    SECTION("string")
    {
        auto tome = Tome::string("hello");
        REQUIRE(fmt::format("{}", tome) == "\"hello\"");
    }
    SECTION("int")
    {
        REQUIRE(fmt::format("{}", Tome::integer((int8_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((int16_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((int32_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((int64_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((uint8_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((uint16_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((uint32_t)42)) == "42");
        REQUIRE(fmt::format("{}", Tome::integer((uint64_t)42)) == "42");
    }
    SECTION("real")
    {
        REQUIRE(fmt::format("{}", Tome::real(3.14f)) == "3.14");
        REQUIRE(fmt::format("{}", Tome::real(3.14)) == "3.14");
    }
    SECTION("complex")
    {
        std::complex<float> c(1.0f, 2.0f);
        REQUIRE(fmt::format("{}", Tome::complex(c)) == "[1,2]");
        REQUIRE(fmt::format("{}", Tome::complex(1.0, 2.0)) == "[1,2]");
    }
    SECTION("dict")
    {
        scribe::Tome::dict_type dict = {{"foo", 42}, {"bar", 3.14}};
        auto tome = Tome::dict(std::move(dict));
        assert(dict.empty()); // should be "moved-from"
        REQUIRE(fmt::format("{}", tome) == R"({"bar":3.14,"foo":42})");
    }
    SECTION("standard array 1D")
    {
        std::vector<Tome> vec = {1, 2, 3};
        auto tome = Tome::array(vec);
        REQUIRE(fmt::format("{}", tome) == "[1,2,3]");
    }
    SECTION("standard array 2d")
    {
        auto tome = Tome::array_from_shape({2, 3});
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
                tome.as_array()(i, j) = i * 3 + j + 1;
        REQUIRE(fmt::format("{}", tome) == "[[1,2,3],[4,5,6]]");
    }
    SECTION("numeric complex array 2d")
    {
        auto tome = Tome::array_from_shape<std::complex<float>>({2, 3});
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 3; j++)
                tome.as_numeric_array<std::complex<float>>()(i, j) =
                    std::complex<float>(i * 3 + j + 1, i * 30 + j * 10);
        REQUIRE(fmt::format("{}", tome) ==
                "[[[1,0],[2,10],[3,20]],[[4,30],[5,40],[6,50]]]");
    }
}

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

struct Point
{
    int x;
    int y;
};

template <> struct scribe::TomeSerializer<Point>
{
    static Tome to_tome(Point const &value)
    {
        Tome r;
        r["x"] = value.x;
        r["y"] = value.y;
        return r;
    }

    static Point from_tome(Tome const &tome)
    {
        Point r;
        r.x = tome["x"].get<int>();
        r.y = tome["y"].get<int>();
        return r;
    }
};

TEST_CASE("custom types to/from tome", "[tome]")
{
    auto p = Point(1, 2);
    Tome tome = p;
    REQUIRE(tome.is_dict());
    auto p2 = tome.get<Point>();
    REQUIRE(p2.x == p.x);
    REQUIRE(p2.y == p.y);
}
