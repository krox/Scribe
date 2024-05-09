#include "scribe/read_json.h"

#include "nlohmann/json.hpp"
#include "scribe/tome.h"
#include <fstream>

namespace scribe {
bool read_json(Tome *tome, nlohmann::json const &j, Schema const &s);

namespace {

bool read_json(Tome *tome, nlohmann::json const &, NoneSchema const &)
{
    (void)tome;
    return false;
}

bool read_json(Tome *tome, nlohmann::json const &, AnySchema const &)
{
    if (tome)
        return false; // AnySchema is not supported for reading data
    return true;
}

bool read_json(Tome *tome, nlohmann::json const &j, BooleanSchema const &)
{
    if (!j.is_boolean())
        return false;

    if (tome)
        *tome = Tome::boolean(j.get<bool>());
    return true;
}

bool read_json(Tome *tome, nlohmann::json const &j, NumberSchema const &s)
{
    if (j.is_number_unsigned())
    {
        if (!s.validate(j.get<uint64_t>()))
            return false;
        if (tome)
            *tome = Tome::integer(j.get<uint64_t>());
        return true;
    }
    else if (j.is_number_integer())
    {
        if (!s.validate(j.get<int64_t>()))
            return false;
        if (tome)
            *tome = Tome::integer(j.get<int64_t>());
        return true;
    }
    else if (j.is_number_float())
    {
        if (!s.validate(j.get<double>()))
            return false;
        if (tome)
            *tome = Tome::real(j.get<double>());
        return true;
    }
    else if (j.is_array() && j.size() == 2 && j[0].is_number_float() &&
             j[1].is_number_float())
    {
        if (!s.validate(j[0].get<double>(), j[1].get<double>()))
            return false;
        if (tome)
            *tome = Tome::complex({j[0].get<double>(), j[1].get<double>()});
        return true;
    }
    else
        return false;
}

bool read_json(Tome *tome, nlohmann::json const &j, StringSchema const &)
{
    if (!j.is_string())
        return false;
    if (tome)
        *tome = Tome::string(j.get<std::string>());
    return true;
}

bool read_json(Tome *tome, nlohmann::json const &j, ArraySchema const &s)
{
    if (!j.is_array())
        return false;
    // TODO: check rank/shape. Also multi-dimensional arrays are not
    // actually supported
    if (tome)
        *tome = Tome::array();
    for (auto const &elem : j)
    {
        if (tome)
            tome->as_array().emplace_back();
        if (!read_json(tome ? &tome->as_array().back() : nullptr, elem,
                       s.elements))
            return false;
    }
    return true;
}

bool read_json(Tome *tome, nlohmann::json const &j, DictSchema const &s)
{
    if (!j.is_object())
        return false;
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
                if (!read_json(tome ? &tome->as_dict()[key] : nullptr, value,
                               s.items[i].schema))
                    return false;

                goto next_item;
            }
        }
        // element (key,value) did not match against any item in the scheme
        return false;
    next_item:;
    }

    // 2) check that all non-optional items were found
    for (size_t i = 0; i < s.items.size(); ++i)
    {
        if (!s.items[i].optional && !found[i])
            return false;
    }

    return true;
}

} // namespace

bool read_json(Tome *tome, nlohmann::json const &j, Schema const &s)
{
    return s.visit<bool>([&](auto const &s) { return read_json(tome, j, s); });
}

} // namespace scribe