#include "catch2/catch_test_macros.hpp"

#include "fmt/format.h"
#include "scribe/codegen.h"

using scribe::Schema;

TEST_CASE("codegen", "[codegen]")
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

    CHECK_NOTHROW(generate_cpp(schema));
}