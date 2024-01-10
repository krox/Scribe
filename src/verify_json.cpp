#include "scribe/verify_json.h"

#include "nlohmann/json.hpp"
#include <fstream>

namespace scribe {
namespace {

bool verify_json(nlohmann::json const &j, Schema const &s);

bool verify_json(nlohmann::json const &, NoneSchema const &) { return false; }

bool verify_json(nlohmann::json const &, AnySchema const &) { return true; }

bool verify_json(nlohmann::json const &j, BooleanSchema const &)
{
    return j.is_boolean();
}

bool verify_json(nlohmann::json const &j, NumberSchema const &s)
{
    switch (s.type)
    {
    case NumType::INT8:
        return j.is_number_integer() &&
               j.get<int64_t>() >= std::numeric_limits<int8_t>::min() &&
               j.get<int64_t>() <= std::numeric_limits<int8_t>::max();
    case NumType::INT16:
        return j.is_number_integer() &&
               j.get<int64_t>() >= std::numeric_limits<int16_t>::min() &&
               j.get<int64_t>() <= std::numeric_limits<int16_t>::max();
    case NumType::INT32:
        return j.is_number_integer() &&
               j.get<int64_t>() >= std::numeric_limits<int32_t>::min() &&
               j.get<int64_t>() <= std::numeric_limits<int32_t>::max();
    case NumType::INT64:
        return j.is_number_integer();
    case NumType::UINT8:
        return j.is_number_integer() &&
               j.get<uint64_t>() <= std::numeric_limits<uint8_t>::max();
    case NumType::UINT16:
        return j.is_number_integer() &&
               j.get<uint64_t>() <= std::numeric_limits<uint16_t>::max();
    case NumType::UINT32:
        return j.is_number_integer() &&
               j.get<uint64_t>() <= std::numeric_limits<uint32_t>::max();
    case NumType::UINT64:
        return j.is_number_integer() &&
               j.get<uint64_t>() <= std::numeric_limits<uint64_t>::max();

    case NumType::FLOAT32:
    case NumType::FLOAT64:
        return j.is_number_float() || j.is_number_integer();

    case NumType::COMPLEX64:
    case NumType::COMPLEX128:
        return j.is_array() && j.size() == 2 && j[0].is_number_float() &&
               j[1].is_number_float();
    }
    assert(false);
}

bool verify_json(nlohmann::json const &j, StringSchema const &)
{
    return j.is_string();
}

bool verify_json(nlohmann::json const &j, ArraySchema const &s)
{
    if (!j.is_array())
        return false;
    // TODO: check rank/shape. Also multi-dimensional arrays are not
    // actually supported
    for (auto const &elem : j)
    {
        if (!verify_json(elem, s.elements))
            return false;
    }
    return true;
}

bool verify_json(nlohmann::json const &j, DictSchema const &s)
{
    if (!j.is_object())
        return false;

    auto found = std::vector<bool>(s.items.size(), false);

    // 1) check all items in the json object against the scheme
    for (auto const &[key, value] : j.items())
    {
        for (size_t i = 0; i < s.items.size(); ++i)
        {
            if (key == s.items[i].key)
            {
                found[i] = true;
                if (!verify_json(value, s.items[i].schema))
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

bool verify_json(nlohmann::json const &j, Schema const &s)
{
    return s.visit<bool>([&](auto const &s) { return verify_json(j, s); });
}

} // namespace

bool verify_json_file(std::string_view filename, Schema const &s, bool verbose)
{
    (void)verbose;
    auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                   nullptr, true, true);
    return verify_json(j, s);
}

} // namespace scribe