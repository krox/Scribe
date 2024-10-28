#pragma once

#include "nlohmann/json.hpp"
#include "scribe/base.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace scribe {

// forward declarations
class Schema;
class SchemaImpl;
class NoneSchema;
class AnySchema;
class BooleanSchema;
class NumberSchema;
class StringSchema;
class ArraySchema;
class DictSchema;

enum class NumType
{
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    COMPLEX_FLOAT32,
    COMPLEX_FLOAT64
};

std::string to_string(NumType type);

struct SchemaMetadata
{
    // unique identifier for the schema (optional)
    // TODO: if given, it might be used to write nested schemas in a more
    // compact way
    std::string name;

    // human-readable description of the schema (optional)
    std::string description;

    // could have 'version', 'author', 'date', etc. Need to think about it more,
    // as some fields might have actual semantics, while other are purely
    // informational.
};

extern const std::shared_ptr<const SchemaImpl> g_schemaimpl_any;

// A schema describes the format of a general data object, called "Tome" in this
// project.
// Design notes:
//   - use shared_ptr internally, while treating all pointees as immutable
//   - the use of shared_ptr itself should be invisible to the user
class Schema
{
    // "shared pointer to const" allows "Schema" to have value semantics while
    // being cheap to copy. Should also be useful for re-using sub-schemas.
    // (i.e. DAG instead of tree)
    std::shared_ptr<const SchemaImpl> schema_ = g_schemaimpl_any; // never null

  public:
    // default-constructed schema is "any"
    Schema() : schema_(g_schemaimpl_any) {}

    explicit Schema(SchemaImpl &&impl)
        : schema_(std::make_shared<SchemaImpl>(std::move(impl)))
    {}

    explicit Schema(NoneSchema s);
    explicit Schema(AnySchema s);
    explicit Schema(BooleanSchema s);
    explicit Schema(NumberSchema s);
    explicit Schema(StringSchema s);
    explicit Schema(ArraySchema s);
    explicit Schema(DictSchema s);

    static Schema from_file(std::string_view filename);
    static Schema from_json(nlohmann::json const &j);

    nlohmann::json to_json() const;

    // shorthand for creating simple schemas
    static Schema none();
    static Schema any();
    static Schema boolean();
    static Schema number(NumType type);
    static Schema string();

    SchemaImpl const &impl() const
    {
        assert(schema_);
        return *schema_;
    }

    template <class R = void, class Visitor> R visit(Visitor &&vis) const;

    std::string_view name() const;
    std::string_view description() const;

    // compares pointer values, not the schema values
    auto operator<=>(const Schema &other) const = default;
};

// compatibility with nlohmann::json '.get' method
void from_json(const nlohmann::json &j, Schema &s);

class NoneSchema
{};

class AnySchema
{};

class BooleanSchema
{};

class NumberSchema
{
  public:
    NumType type;
    // future: some min/max values could go here

    bool is_integer() const;
    bool is_real() const;
    bool is_complex() const;

    // validate a integer/real/complex number against the schema
    void validate(int64_t) const;
    void validate(double) const;
    void validate(double, double) const;
};

class StringSchema
{
  public:
    std::optional<int> min_length;
    std::optional<int> max_length;
    // future: full regex pattern could go here

    void validate(std::string_view) const;
};

class ArraySchema
{
  public:
    Schema elements;

    std::optional<std::vector<int64_t>> shape;

    void validate_shape(std::span<const size_t> shape) const;
};

struct ItemSchema
{
    std::string key;
    Schema schema;
    bool optional = false;
};

class DictSchema
{
    // -1 if not found
    int find_key(std::string_view key) const;

  public:
    std::vector<ItemSchema> items;

    // Validate that each (non-optional) key is present in the given list of
    // keys. returns the schema that each sub-object should be validated
    // against.
    std::vector<Schema> validate(std::span<const std::string> keys) const;
};

class SchemaImpl
{
  public:
    std::variant<NoneSchema, AnySchema, BooleanSchema, NumberSchema,
                 StringSchema, ArraySchema, DictSchema>
        schema_;
    SchemaMetadata metadata_ = {}; // all optional
};

template <class R, class Visitor> R Schema::visit(Visitor &&vis) const
{
    return std::visit<R>(std::forward<Visitor>(vis), impl().schema_);
}

inline std::string_view Schema::name() const { return impl().metadata_.name; }
inline std::string_view Schema::description() const
{
    return impl().metadata_.description;
}

} // namespace scribe