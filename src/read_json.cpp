#include "scribe/read_json.h"

#include "nlohmann/json.hpp"
#include "scribe/tome.h"
#include <fstream>

namespace scribe {
void read_json(Tome *tome, nlohmann::json const &j, Schema const &s);

namespace {

void read_json(Tome *tome, nlohmann::json const &, NoneSchema const &)
{
    (void)tome;
    throw ValidationError("NoneSchema is never valid");
}

void read_json(Tome *tome, nlohmann::json const &, AnySchema const &)
{
    if (tome)
        throw ReadError("AnySchema cannot be read into a Tome");
}

void read_json(Tome *tome, nlohmann::json const &j, BooleanSchema const &)
{
    if (!j.is_boolean())
        throw ValidationError("expected boolean");

    if (tome)
        *tome = Tome::boolean(j.get<bool>());
}

void read_json(Tome *tome, nlohmann::json const &j, NumberSchema const &s)
{
    if (j.is_number_unsigned())
    {
        if (!s.validate(j.get<uint64_t>()))
            throw ValidationError("unsigned integer out of range");
        if (tome)
            *tome = Tome::integer(j.get<uint64_t>());
    }
    else if (j.is_number_integer())
    {
        if (!s.validate(j.get<int64_t>()))
            throw ValidationError("signed integer out of range");
        if (tome)
            *tome = Tome::integer(j.get<int64_t>());
    }
    else if (j.is_number_float())
    {
        if (!s.validate(j.get<double>()))
            throw ValidationError("real number out of range");
        if (tome)
            *tome = Tome::real(j.get<double>());
    }
    else if (j.is_array() && j.size() == 2 && j[0].is_number_float() &&
             j[1].is_number_float())
    {
        if (!s.validate(j[0].get<double>(), j[1].get<double>()))
            throw ValidationError("complex number out of range");
        if (tome)
            *tome = Tome::complex({j[0].get<double>(), j[1].get<double>()});
    }
    else
    {
        throw ValidationError("expected number");
    }
}

void read_json(Tome *tome, nlohmann::json const &j, StringSchema const &)
{
    if (!j.is_string())
        throw ValidationError("expected string");
    if (tome)
        *tome = Tome::string(j.get<std::string>());
}

void read_json(Tome *tome, nlohmann::json const &j, ArraySchema const &s)
{
    if (!j.is_array())
        throw ValidationError("expected array");
    // TODO: check rank/shape. Also multi-dimensional arrays are not
    // actually supported
    if (tome)
        *tome = Tome::array();
    for (auto const &elem : j)
    {
        if (tome)
            tome->as_array().emplace_back();
        read_json(tome ? &tome->as_array().back() : nullptr, elem, s.elements);
    }
}

void read_json(Tome *tome, nlohmann::json const &j, DictSchema const &s)
{
    if (!j.is_object())
        throw ValidationError("expected object");
    if (tome)
        *tome = Tome::dict();

    auto found = std::vector<bool>(s.items.size(), false);

    // 1) check all items in the json object against the scheme and read them
    for (auto const &[key, value] : j.items())
    {
        for (size_t i = 0; i < s.items.size(); ++i)
        {
            if (key == s.items[i].key)
            {
                found[i] = true;
                read_json(tome ? &tome->as_dict()[key] : nullptr, value,
                          s.items[i].schema);

                goto next_item;
            }
        }
        // element (key,value) did not match against any item in the scheme
        throw ValidationError("unexpected key: " + key);
    next_item:;
    }

    // 2) check that all non-optional items were found
    for (size_t i = 0; i < s.items.size(); ++i)
    {
        if (!s.items[i].optional && !found[i])
            throw ValidationError("missing key: " + s.items[i].key);
    }
}

} // namespace

void read_json(Tome *tome, nlohmann::json const &j, Schema const &s)
{
    s.visit([&](auto const &s) { read_json(tome, j, s); });
}

} // namespace scribe