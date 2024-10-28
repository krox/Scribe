#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/tome.h"

namespace scribe {
namespace internal {

// validates and reads a JSON object according to the given schema
//   * throws ValidationError if the JSON object does not follow the schema
//   * set tome=nullptr to only validate
void read_json(Tome *, nlohmann::json const &, Schema const &);

// writes a JSON object according to the given schema
void write_json(nlohmann::json &, Tome const &, Schema const &);

} // namespace internal

inline void read_json(bool &data, nlohmann::json const &json)
{
    if (!json.is_boolean())
        throw ValidationError("expected boolean");
    data = json.get<bool>();
}

inline void read_json(std::string &data, nlohmann::json const &json)
{
    if (!json.is_string())
        throw ValidationError("expected string");
    data = json.get<std::string>();
}

template <IntegerType T> void read_json(T &data, nlohmann::json const &json)
{
    if (!json.is_number_integer())
        throw ValidationError("expected integer");
    data = json.get<T>();
}

template <RealType T> void read_json(T &data, nlohmann::json const &json)
{
    if (!json.is_number())
        throw ValidationError("expected floating point number, got " +
                              json.dump());
    data = json.get<T>();
}

template <class T>
void read_json(std::vector<T> &data, nlohmann::json const &json)
{
    if (!json.is_array())
        throw ValidationError("expected array");
    data.clear();
    T value;
    for (auto const &item : json)
    {
        read_json(value, item);
        data.push_back(value);
    }
}

} // namespace scribe