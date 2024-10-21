#include "scribe/schema.h"

#include "fmt/format.h"
#include <fstream>

std::string scribe::to_string(NumType type)
{
    switch (type)
    {
    case NumType::INT8:
        return "int8";
    case NumType::INT16:
        return "int16";
    case NumType::INT32:
        return "int32";
    case NumType::INT64:
        return "int64";
    case NumType::UINT8:
        return "uint8";
    case NumType::UINT16:
        return "uint16";
    case NumType::UINT32:
        return "uint32";
    case NumType::UINT64:
        return "uint64";
    case NumType::FLOAT32:
        return "float32";
    case NumType::FLOAT64:
        return "float64";
    case NumType::COMPLEX_FLOAT32:
        return "complex_float32";
    case NumType::COMPLEX_FLOAT64:
        return "complex_float64";
    default:
        throw std::runtime_error("unknown NumType");
    }
}

scribe::Schema::Schema(NoneSchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(AnySchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(BooleanSchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(NumberSchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(StringSchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(ArraySchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}
scribe::Schema::Schema(DictSchema s)
    : schema_(std::make_shared<SchemaImpl>(std::move(s)))
{}

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
    else if (type == "complex_float32")
    {
        s.schema_ = NumberSchema{.type = NumType::COMPLEX_FLOAT32};
    }
    else if (type == "complex_float64")
    {
        s.schema_ = NumberSchema{.type = NumType::COMPLEX_FLOAT64};
    }
    else if (type == "string")
    {
        StringSchema string_schema;
        get_optional(string_schema.min_length, "min_length");
        get_optional(string_schema.max_length, "max_length");
        s.schema_ = string_schema;
    }
    else if (type == "array")
    {
        ArraySchema array_schema;
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

nlohmann::json Schema::to_json() const
{
    nlohmann::json j;
    if (name() != "")
        j["schema_name"] = name();
    if (description() != "")
        j["schema_description"] = description();

    this->visit(overloaded{
        [&](NoneSchema const &) { j["type"] = "none"; },
        [&](AnySchema const &) { j["type"] = "any"; },
        [&](BooleanSchema const &) { j["type"] = "bool"; },
        [&](NumberSchema const &s) { j["type"] = to_string(s.type); },
        [&](StringSchema const &s) {
            j["type"] = "string";
            if (s.min_length)
                j["min_length"] = *s.min_length;
            if (s.max_length)
                j["max_length"] = *s.max_length;
        },
        [&](ArraySchema const &s) {
            j["type"] = "array";
            if (s.shape)
                j["shape"] = *s.shape;
            j["elements"] = s.elements.to_json();
        },
        [&](DictSchema const &s) {
            j["type"] = "dict";
            j["items"] = nlohmann::json::array();
            for (auto const &item : s.items)
            {
                nlohmann::json item_j;
                item_j["key"] = item.key;
                if (item.optional)
                    item_j["optional"] = true;
                item_j.merge_patch(item.schema.to_json());
                j["items"].push_back(item_j);
            }
        }});
    return j;
}

Schema Schema::none()
{
    Schema s;
    s.schema_ = std::make_shared<SchemaImpl>(SchemaImpl{NoneSchema{}});
    return s;
}

Schema Schema::any()
{
    Schema s;
    s.schema_ = g_schemaimpl_any;
    return s;
}

Schema Schema::boolean()
{
    Schema s;
    s.schema_ = std::make_shared<SchemaImpl>(BooleanSchema{});
    return s;
}

Schema Schema::number(NumType type)
{
    Schema s;
    s.schema_ = std::make_shared<SchemaImpl>(NumberSchema{.type = type});
    return s;
}

Schema Schema::string()
{
    Schema s;
    s.schema_ = std::make_shared<SchemaImpl>(StringSchema{});
    return s;
}

void NumberSchema::validate(int64_t value) const
{
    switch (type)
    {
    case NumType::INT8:
        if (value < std::numeric_limits<int8_t>::min() ||
            value > std::numeric_limits<int8_t>::max())
            throw ValidationError("integer value out of range of int8");
        break;
    case NumType::INT16:
        if (value < std::numeric_limits<int16_t>::min() ||
            value > std::numeric_limits<int16_t>::max())
            throw ValidationError("integer value out of range of int16");
        break;
    case NumType::INT32:
        if (value < std::numeric_limits<int32_t>::min() ||
            value > std::numeric_limits<int32_t>::max())
            throw ValidationError("integer value out of range of int32");
        break;
    case NumType::INT64:
        break;
    case NumType::UINT8:
        if (value < 0 || value > std::numeric_limits<uint8_t>::max())
            throw ValidationError("integer value out of range of uint8");
        break;
    case NumType::UINT16:
        if (value < 0 || value > std::numeric_limits<uint16_t>::max())
            throw ValidationError("integer value out of range of uint16");
        break;
    case NumType::UINT32:
        if (value < 0 || value > std::numeric_limits<uint32_t>::max())
            throw ValidationError("integer value out of range of uint32");
        break;
    case NumType::UINT64:
        if (value < 0)
            throw ValidationError("integer value out of range of uint64");
        break;
    case NumType::FLOAT32:
    case NumType::FLOAT64:
    case NumType::COMPLEX_FLOAT32:
    case NumType::COMPLEX_FLOAT64:
        break;

    default:
        throw std::runtime_error("invalid NumType");
    }
}

void NumberSchema::validate(double) const
{
    switch (type)
    {
    case NumType::INT8:
    case NumType::INT16:
    case NumType::INT32:
    case NumType::INT64:
    case NumType::UINT8:
    case NumType::UINT16:
    case NumType::UINT32:
    case NumType::UINT64:
        throw ValidationError("expected integer, got real number");
    case NumType::FLOAT32:
    case NumType::FLOAT64:
    case NumType::COMPLEX_FLOAT32:
    case NumType::COMPLEX_FLOAT64:
        break;
    default:
        throw std::runtime_error("invalid NumType");
    }
}

void NumberSchema::validate(double, double) const
{
    switch (type)
    {
    case NumType::INT8:
    case NumType::INT16:
    case NumType::INT32:
    case NumType::INT64:
    case NumType::UINT8:
    case NumType::UINT16:
    case NumType::UINT32:
    case NumType::UINT64:
        throw ValidationError("expected integer, got complex");
    case NumType::FLOAT32:
    case NumType::FLOAT64:
        throw ValidationError("expected real number, got complex");
    case NumType::COMPLEX_FLOAT32:
    case NumType::COMPLEX_FLOAT64:
        break;
    default:
        throw std::runtime_error("invalid NumType");
    }
}

bool NumberSchema::is_integer() const
{
    switch (type)
    {
    case NumType::INT8:
    case NumType::INT16:
    case NumType::INT32:
    case NumType::INT64:
    case NumType::UINT8:
    case NumType::UINT16:
    case NumType::UINT32:
    case NumType::UINT64:
        return true;
    default:
        return false;
    }
}

bool NumberSchema::is_real() const
{
    switch (type)
    {
    case NumType::FLOAT32:
    case NumType::FLOAT64:
        return true;
    default:
        return false;
    }
}

bool NumberSchema::is_complex() const
{
    switch (type)
    {
    case NumType::COMPLEX_FLOAT32:
    case NumType::COMPLEX_FLOAT64:
        return true;
    default:
        return false;
    }
}

void StringSchema::validate(std::string_view value) const
{
    if (min_length && (int)value.size() < *min_length)
        throw ValidationError("string too short");
    if (max_length && (int)value.size() > *max_length)
        throw ValidationError("string too long");
}

void ArraySchema::validate_shape(std::span<const size_t> shape) const
{
    if (this->shape)
    {
        if (shape.size() != this->shape->size())
            throw ValidationError(
                "shape mismatch (wrong number of dimensions)");
        for (size_t i = 0; i < shape.size(); ++i)
            if ((*this->shape)[i] != -1 &&
                shape[i] != (size_t)(*this->shape)[i])
                throw ValidationError("shape mismatch (wrong size)");
    }
}

int DictSchema::find_key(std::string_view key) const
{
    for (size_t i = 0; i < items.size(); ++i)
        if (key == items[i].key)
            return i;
    return -1;
}

std::vector<Schema>
DictSchema::validate(std::span<const std::string> keys) const
{
    auto found = std::vector<bool>(items.size(), false);
    std::vector<Schema> schemas;
    for (auto const &key : keys)
    {
        int i = find_key(key);
        if (i == -1)
            throw ValidationError("unexpected key: " + key);
        found[i] = true;
        schemas.push_back(items[i].schema);
    }

    for (size_t i = 0; i < items.size(); ++i)
    {
        if (!items[i].optional && !found[i])
            throw ValidationError("missing key: " + items[i].key);
    }
    return schemas;
}

} // namespace scribe