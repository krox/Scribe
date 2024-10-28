#include "scribe/codegen.h"

#include "fmt/format.h"

namespace scribe {
namespace {

class Codegen
{
    std::vector<std::string> source_forward_;
    std::vector<std::string> source_header_;
    std::vector<std::string> source_impl_;
    int anon_count = 0;

    std::map<Schema, std::string> type_cache_;

    // types not yet generated
    std::vector<std::tuple<Schema, std::string>> todo_list_;

    std::string get_array_type(ArraySchema const &schema)
    {
        // TODO: this is wrong. obviously.
        std::string element_type = get_type(schema.elements);
        return fmt::format("std::vector<{}>", element_type);
    }

    std::string get_dict_type(DictSchema const &schema, std::string_view name_)
    {
        std::string name = name_.empty()
                               ? fmt::format("anon_struct_{}", anon_count++)
                               : std::string(name_);
        source_forward_.push_back(fmt::format("struct {};", name));

        std::string header = fmt::format("struct {} {{\n", name);
        for (auto const &item : schema.items)
        {
            auto item_type = get_type(item.schema);
            if (item.optional)
                item_type = fmt::format("std::optional<{}>", item_type);
            header += fmt::format("    {} {};\n", item_type, item.key);
        }
        header += "};\n";

        source_header_.push_back(header);
        return name;
    }

  public:
    std::string get_type(Schema const &schema)
    {
        auto it = type_cache_.find(schema);
        if (it != type_cache_.end())
            return it->second;
        std::string name = schema.visit<std::string>(overloaded{
            [](NoneSchema const &) -> std::string {
                throw std::runtime_error("cannot generate 'None' type");
            },
            [](AnySchema const &) -> std::string { return "scribe::Tome"; },
            [](BooleanSchema const &) -> std::string { return "bool"; },
            [](StringSchema const &) -> std::string { return "bool"; },
            [](NumberSchema const &schema) -> std::string {
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
            },
            [&](ArraySchema const &schema) -> std::string {
                return get_array_type(schema);
            },
            [&, dict_name = schema.name()](DictSchema const &s) -> std::string {
                return get_dict_type(s, dict_name);
            }});
        return type_cache_[schema] = name;
    }

    std::string get_source() const
    {
        return fmt::format("#include <scribe/tome.h>\n\n{}\n{}\n",
                           fmt::join(source_header_, "\n"),
                           fmt::join(source_impl_, "\n"));
    }
};

} // namespace

std::string generate_cpp(Schema const &schema)
{
    Codegen c;
    c.get_type(schema);
    return c.get_source();
}
} // namespace scribe