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
    std::vector<
        std::tuple<std::reference_wrapper<const DictSchema>, std::string>>
        todo_list_;

    void generate_type(DictSchema const &schema, std::string_view name);
    void generate_implementation(DictSchema const &schema,
                                 std::string_view name);

  public:
    void generate_all()
    {
        while (!todo_list_.empty())
        {
            auto [schema, name] = todo_list_.back();
            todo_list_.pop_back();
            // generate_type(schema.get(), name);
            generate_implementation(schema.get(), name);
        }
    }

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
            [](StringSchema const &) -> std::string { return "std::string"; },
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
                std::string element_type = get_type(schema.elements);
                return fmt::format("scribe::Array<{}>", element_type);
            },
            [&](DictSchema const &s) -> std::string {
                std::string name =
                    schema.name().empty()
                        ? fmt::format("anon_struct_{}", anon_count++)
                        : std::string(schema.name());
                todo_list_.push_back(std::tuple(std::cref(s), name));
                generate_type(s, name);
                return name;
            }});
        return type_cache_[schema] = name;
    }

    std::string get_source() const
    {
        if (!todo_list_.empty())
            throw std::runtime_error("unresolved types (should call "
                                     ".generate_all() before .get_source())");

        return fmt::format("#include \"scribe/scribe.h\"\n"
                           "\n{}\n\n{}\n\n{}\n",
                           fmt::join(source_forward_, "\n"),
                           fmt::join(source_header_, "\n"),
                           fmt::join(source_impl_, "\n"));
    }
};

void Codegen::generate_type(DictSchema const &schema, std::string_view name)
{
    // forward declaration
    source_forward_.push_back(fmt::format("struct {};", name));

    // type definition
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
}
void Codegen::generate_implementation(DictSchema const &schema,
                                      std::string_view name)
{
    std::string impl;
    auto it = std::back_inserter(impl);
    fmt::format_to(it,
                   "void read({} & data, scribe::Reader auto& reader) {{\n"
                   "    using scribe::read;\n",
                   name);
    for (auto const &item : schema.items)
    {
        fmt::format_to(it, "    read(data.{0}, reader, \"{0}\");\n", item.key);
    }
    fmt::format_to(it, "}}\n");

    source_impl_.push_back(impl);
}

} // namespace

std::string generate_cpp(Schema const &schema)
{
    Codegen c;
    c.get_type(schema);
    c.generate_all();
    return c.get_source();
}
} // namespace scribe