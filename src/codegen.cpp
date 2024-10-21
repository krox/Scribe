#include "scribe/codegen.h"

#include "fmt/format.h"

namespace scribe {
namespace {

struct Codegen
{
    std::string src;
    int anon_count = 0;

    std::string get_type(Schema const &schema);
    std::string get_type(std::string_view name, NoneSchema const &);
    std::string get_type(std::string_view name, AnySchema const &);
    std::string get_type(std::string_view name, BooleanSchema const &);
    std::string get_type(std::string_view name, NumberSchema const &);
    std::string get_type(std::string_view name, StringSchema const &);
    std::string get_type(std::string_view name, ArraySchema const &);
    std::string get_type(std::string_view name, DictSchema const &);
};

inline std::string Codegen::get_type(Schema const &schema)
{
    return schema.visit<std::string>(
        [&](auto const &s) { return get_type(schema.name(), s); });
}

inline std::string Codegen::get_type(std::string_view, NoneSchema const &)
{
    throw std::runtime_error("cannot generate 'None' type");
}

inline std::string Codegen::get_type(std::string_view, AnySchema const &)
{
    throw std::runtime_error("'Any' type not implemented in codegen");
}

inline std::string Codegen::get_type(std::string_view, BooleanSchema const &)
{
    return "bool";
}

inline std::string Codegen::get_type(std::string_view,
                                     NumberSchema const &schema)
{
    switch (schema.type)
    {
    case NumType::INT8:
        return "int8_t";
    case NumType::INT16:
        return "int16_t";
    case NumType::INT32:
        return "int32_t";
    case NumType::INT64:
        return "int64_t";
    case NumType::UINT8:
        return "uint8_t";
    case NumType::UINT16:
        return "uint16_t";
    case NumType::UINT32:
        return "uint32_t";
    case NumType::UINT64:
        return "uint64_t";
    case NumType::FLOAT32:
        return "float";
    case NumType::FLOAT64:
        return "double";
    case NumType::COMPLEX_FLOAT32:
        return "std::complex<float>";
    case NumType::COMPLEX_FLOAT64:
        return "std::complex<double>";
    default:
        throw std::runtime_error("invalid NumType");
    }
}

inline std::string Codegen::get_type(std::string_view, StringSchema const &)
{
    return "std::string";
}

inline std::string Codegen::get_type(std::string_view, ArraySchema const &s)
{
    return fmt::format("std::vector<{}>", get_type(s.elements));
}

inline std::string Codegen::get_type(std::string_view name_,
                                     DictSchema const &schema)
{
    auto name = name_.empty() ? fmt::format("anon_{}", ++anon_count)
                              : std::string(name_);

    src += fmt::format("struct {} {{\n", name);
    for (auto const &item : schema.items)
    {
        auto item_type = get_type(item.schema);
        if (item.optional)
            item_type = fmt::format("std::optional<{}>", item_type);
        src += fmt::format("    {} {};\n", item_type, item.key);
    }
    src += "};\n";
    return std::string(name);
}

} // namespace

std::string generate_cpp(Schema const &schema)
{
    Codegen c;
    c.get_type(schema);
    return c.src;
}
} // namespace scribe