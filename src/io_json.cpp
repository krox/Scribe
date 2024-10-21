#include "scribe/io_json.h"

#include "nlohmann/json.hpp"
#include "scribe/tome.h"
#include <fstream>

namespace {
using namespace scribe;

void read_impl(Tome *tome, nlohmann::json const &, NoneSchema const &)
{
    (void)tome;
    throw ValidationError("NoneSchema is never valid");
}

void read_impl(Tome *tome, nlohmann::json const &, AnySchema const &)
{
    if (tome)
        throw ReadError("AnySchema cannot be read into a Tome");
}

void read_impl(Tome *tome, nlohmann::json const &j, BooleanSchema const &)
{
    if (!j.is_boolean())
        throw ValidationError("expected boolean");

    if (tome)
        *tome = Tome::boolean(j.get<bool>());
}

void read_impl(Tome *tome, nlohmann::json const &j, NumberSchema const &s)
{
    if (j.is_number_integer())
    {
        auto value = j.get<int64_t>();
        s.validate(value);
        if (tome)
            *tome = Tome::number_unchecked(value, s.type);
    }
    else if (j.is_number_float())
    {
        auto value = j.get<double>();
        s.validate(value);
        if (tome)
            *tome = Tome::number_unchecked(value, s.type);
    }
    else
        throw ValidationError("expected number");
}

void read_impl(Tome *tome, nlohmann::json const &j, StringSchema const &s)
{
    if (!j.is_string())
        throw ValidationError("expected string");
    auto value = j.get<std::string>();
    s.validate(value);
    if (tome)
        *tome = value;
}

void read_elements(std::vector<Tome> *elements, nlohmann::json const &j,
                   Schema const &s, int dim, std::vector<int64_t> &shape)
{
    if (dim == (int)shape.size())
    {
        if (elements)
        {
            elements->emplace_back();
            internal::read_json(&elements->back(), j, s);
        }
        else
        {
            internal::read_json(nullptr, j, s);
        }
        return;
    }

    if (!j.is_array())
        throw ValidationError("expected array");
    if (shape[dim] == -1)
        shape[dim] = j.size();
    if ((int)j.size() != shape[dim])
        throw ValidationError("expected array of size " +
                              std::to_string(shape[dim]));

    for (auto const &elem : j)
        read_elements(elements, elem, s, dim + 1, shape);
}

void read_impl(Tome *tome, nlohmann::json const &j, ArraySchema const &s)
{
    if (!s.shape)
        throw ReadError(
            "ArraySchema without shape cannot be read/validated from JSON");
    auto shape = *s.shape;

    if (tome)
    {
        std::vector<Tome> elements;
        read_elements(&elements, j, s.elements, 0, shape);
        *tome = Tome::array(std::move(elements),
                            std::vector<size_t>(shape.begin(), shape.end()));
    }
    else
    {
        read_elements(nullptr, j, s.elements, 0, shape);
    }
}

void read_impl(Tome *tome, nlohmann::json const &j, DictSchema const &s)
{
    if (!j.is_object())
        throw ValidationError("expected object");

    // get and validate list of keys
    std::vector<std::string> keys;
    for (auto const &item : j.items())
        keys.push_back(item.key());
    auto schemas = s.validate(keys);
    assert(keys.size() == schemas.size());

    // read and validate each item
    if (tome)
        *tome = Tome::dict();
    for (size_t i = 0; i < keys.size(); ++i)
        internal::read_json(tome ? &tome->as_dict()[keys[i]] : nullptr,
                            j.at(keys[i]), schemas[i]);
}

void write_impl(nlohmann::json &, Tome const &, NoneSchema const &)
{
    throw ValidationError("NoneSchema is never valid");
}

void write_elements(nlohmann::json &j, Tome const *&elements, Schema const &s,
                    int dim, std::vector<size_t> const &shape)
{
    if (dim == (int)shape.size())
    {
        internal::write_json(j, *elements++, s);
        return;
    }

    j = nlohmann::json::array();

    for (size_t i = 0; i < shape[dim]; ++i)
    {
        j.push_back(nullptr);
        write_elements(j[i], elements, s, dim + 1, shape);
    }
}

void write_impl(nlohmann::json &j, Tome const &tome, AnySchema const &)
{
    if (tome.is_boolean())
        j = tome.as_boolean();
    else if (tome.is_integer())
        j = tome.get<int64_t>();
    else if (tome.is_real())
        j = tome.get<double>();
    else if (tome.is_complex())
    {
        auto value = tome.get<std::complex<double>>();
        j = {value.real(), value.imag()};
    }
    else if (tome.is_string())
        j = tome.as_string();
    else if (tome.is_dict())
    {
        auto dict = tome.as_dict();
        j = nlohmann::json::object();
        for (auto const &item : dict)
            write_impl(j[item.first], item.second, AnySchema{});
    }
    else if (tome.is_array())
    {
        auto array = tome.as_array();
        j = nlohmann::json::array();
        Tome const *it = &*array.linear_begin();
        write_elements(j, it, Schema::any(), 0, tome.shape());
    }
    else
        throw WriteError("unsupported type when writing AnySchema to JSON");
}

void write_impl(nlohmann::json &j, Tome const &tome, BooleanSchema const &)
{
    if (!tome.is_boolean())
        throw ValidationError("expected boolean");
    j = tome.as_boolean();
}

void write_impl(nlohmann::json &j, Tome const &tome, NumberSchema const &s)
{
    if (tome.is_integer())
    {
        auto val = tome.get<int64_t>();
        s.validate(val);
        j = val;
    }
    else if (tome.is_real())
    {
        auto val = tome.get<double>();
        s.validate(val);
        j = val;
    }
    else if (tome.is_complex())
    {
        auto val = tome.get<std::complex<double>>();
        s.validate(val.real(), val.imag());
        j = {val.real(), val.imag()};
    }
    else
        throw ValidationError("expected number");
}

void write_impl(nlohmann::json &j, Tome const &tome, StringSchema const &)
{
    j = tome.as_string();
}

void write_impl(nlohmann::json &j, Tome const &tome, ArraySchema const &s)
{
    if (!tome.is_array())
        throw ValidationError("expected array");
    auto const &a = tome.as_array();
    auto shape = tome.shape();

    Tome const *it = &*a.linear_begin();
    write_elements(j, it, s.elements, 0, shape);
}

void write_impl(nlohmann::json &j, Tome const &tome, DictSchema const &s)
{
    if (!tome.is_dict())
        throw ValidationError("expected dict");
    auto const &d = tome.as_dict();

    for (auto const &item : s.items)
    {
        auto it = d.find(item.key);
        if (it == d.end())
        {
            if (!item.optional)
                throw ValidationError("missing key: " + item.key);
            continue;
        }

        internal::write_json(j[item.key], it->second, item.schema);
    }
}
} // namespace

void scribe::internal::read_json(Tome *tome, nlohmann::json const &j,
                                 Schema const &s)
{
    s.visit([&](auto const &s) { read_impl(tome, j, s); });
}

void scribe::internal::write_json(nlohmann::json &j, Tome const &tome,
                                  Schema const &s)
{
    s.visit([&](auto const &s) { write_impl(j, tome, s); });
}
