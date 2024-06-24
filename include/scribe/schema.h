#pragma once

#include "nlohmann/json.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace scribe {

struct ScribeError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// thrown when a data file/object does not follow the expected schema
struct ValidationError : ScribeError
{
    using ScribeError::ScribeError;
};

// thrown when a data file/object cannot be read
struct ReadError : ScribeError
{
    using ScribeError::ScribeError;
};

// helper for std::visit
template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
    COMPLEX64,
    COMPLEX128
};

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
    // TODO: clarify the semantics of default construction (should be either
    // "any" or "none" I guess)
    Schema() : schema_(g_schemaimpl_any) {}

    explicit Schema(SchemaImpl &&impl)
        : schema_(std::make_shared<SchemaImpl>(std::move(impl)))
    {}

    static Schema from_file(std::string_view filename);
    static Schema from_json(nlohmann::json const &j);

    SchemaImpl const &impl() const
    {
        assert(schema_);
        return *schema_;
    }

    template <class R = void, class Visitor> R visit(Visitor &&vis) const;

    std::string_view name() const;
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
    // TODO: min/max

    // validate a integer/real/complex number against the schema
    bool validate(int64_t) const;
    bool validate(uint64_t) const;
    bool validate(double) const;
    bool validate(double, double) const;
};

class StringSchema
{
    // TODO: a full regex-style constraint could go here
};

class ArraySchema
{
  public:
    Schema elements;

    std::optional<int> rank;
    std::optional<std::vector<int>> shape;
};

struct ItemSchema
{
    std::string key;
    Schema schema;
    bool optional = false;
};

class DictSchema
{
  public:
    std::vector<ItemSchema> items;
};

class SchemaImpl
{
  public:
    std::variant<NoneSchema, AnySchema, BooleanSchema, NumberSchema,
                 StringSchema, ArraySchema, DictSchema>
        schema_;
    SchemaMetadata metadata_; // all optional
};

template <class R, class Visitor> R Schema::visit(Visitor &&vis) const
{
    return std::visit<R>(std::forward<Visitor>(vis), impl().schema_);
}

inline std::string_view Schema::name() const { return impl().metadata_.name; }

} // namespace scribe