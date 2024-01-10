#include "scribe/schema.h"

#include "fmt/format.h"
#include <fstream>

namespace scribe {

const std::shared_ptr<const SchemaImpl> g_schemaimpl_any =
    std::make_shared<SchemaImpl>(AnySchema{});

Schema Schema::from_file(std::string_view filename)
{
    return Schema::from_json(nlohmann::json::parse(
        std::ifstream(std::string(filename)), /*callback=*/nullptr,
        /*allow_exception=*/true, /*allow_comments=*/true));
}

Schema Schema::from_json(nlohmann::json const &j)
{
    auto get_optional = [&j]<class T>(std::optional<T> &r,
                                      std::string_view key) -> void {
        if (!j.contains(key) || j.at(key).is_null())
            r = std::nullopt;
        else
            r = j.at(key).get<T>();
    };

    SchemaImpl s;

    {
        SchemaMetadata meta;
        meta.name = j.value<std::string>("schema_name", "");
        meta.description = j.value<std::string>("schema_description", "");
        s.metadata_ = meta;
    }

    auto type = j.value("type", "any");
    if (type == "none")
    {
        s.schema_ = NoneSchema();
    }
    else if (type == "any")
    {
        s.schema_ = AnySchema();
    }
    else if (type == "bool")
    {
        s.schema_ = BooleanSchema();
    }
    else if (type == "int8")
    {
        s.schema_ = NumberSchema{.type = NumType::INT8};
    }
    else if (type == "int16")
    {
        s.schema_ = NumberSchema{.type = NumType::INT16};
    }
    else if (type == "int32")
    {
        s.schema_ = NumberSchema{.type = NumType::INT32};
    }
    else if (type == "int64")
    {
        s.schema_ = NumberSchema{.type = NumType::INT64};
    }
    else if (type == "uint8")
    {
        s.schema_ = NumberSchema{.type = NumType::UINT8};
    }
    else if (type == "uint16")
    {
        s.schema_ = NumberSchema{.type = NumType::UINT16};
    }
    else if (type == "uint32")
    {
        s.schema_ = NumberSchema{.type = NumType::UINT32};
    }
    else if (type == "uint64")
    {
        s.schema_ = NumberSchema{.type = NumType::UINT64};
    }
    else if (type == "float32")
    {
        s.schema_ = NumberSchema{.type = NumType::FLOAT32};
    }
    else if (type == "float64")
    {
        s.schema_ = NumberSchema{.type = NumType::FLOAT64};
    }
    else if (type == "complex64")
    {
        s.schema_ = NumberSchema{.type = NumType::COMPLEX64};
    }
    else if (type == "complex128")
    {
        s.schema_ = NumberSchema{.type = NumType::COMPLEX128};
    }
    else if (type == "string")
    {
        s.schema_ = StringSchema();
    }
    else if (type == "array")
    {
        ArraySchema array_schema;
        get_optional(array_schema.rank, "rank");
        get_optional(array_schema.shape, "shape");
        array_schema.elements = Schema::from_json(j.at("elements"));
        s.schema_ = array_schema;
    }
    else if (type == "dict")
    {
        DictSchema dict_schema;

        for (auto const &item : j.at("items"))
        {
            ItemSchema item_schema;
            item_schema.key = item.at("key").get<std::string>();
            item_schema.optional = item.value<bool>("optional", false);
            item_schema.schema = Schema::from_json(item);
            dict_schema.items.push_back(item_schema);
        }

        s.schema_ = dict_schema;
    }
    else
    {
        throw std::runtime_error(fmt::format("unknown schema type '{}'", type));
    }

    return Schema(std::move(s));
}
} // namespace scribe