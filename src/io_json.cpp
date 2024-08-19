#include "scribe/io_json.h"

#include "nlohmann/json.hpp"
#include "scribe/tome.h"
#include <fstream>

namespace scribe {
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
    switch (s.type)
    {
    case NumType::INT8:
    case NumType::INT16:
    case NumType::INT32:
    case NumType::INT64:
    case NumType::UINT8:
    case NumType::UINT16:
    case NumType::UINT32:
    case NumType::UINT64:
        if (!j.is_number_integer())
            throw ValidationError("expected integer");
        s.validate(j.get<int64_t>());

        if (tome)
            *tome = Tome::integer(j.get<int64_t>());
        break;

    case NumType::FLOAT32:
    case NumType::FLOAT64:
        if (j.is_number_integer())
        {
            s.validate(j.get<int64_t>());
            if (tome)
                *tome = Tome::real(j.get<int64_t>());
        }
        else if (j.is_number_float())
        {
            s.validate(j.get<double>());
            if (tome)
                *tome = Tome::real(j.get<double>());
        }
        else
            throw ValidationError("expected real number");
        break;

    case NumType::COMPLEX64:
    case NumType::COMPLEX128:
        if (!j.is_array() || j.size() != 2 || !j[0].is_number_float() ||
            !j[1].is_number_float())
            throw ValidationError("expected complex number");
        s.validate(j[0].get<double>(), j[1].get<double>());

        if (tome)
            *tome = Tome::complex({j[0].get<double>(), j[1].get<double>()});
        break;
    default:
        throw std::runtime_error("invalid NumType");
    }
}

void read_json(Tome *tome, nlohmann::json const &j, StringSchema const &)
{
    if (!j.is_string())
        throw ValidationError("expected string");
    if (tome)
        *tome = Tome::string(j.get<std::string>());
}

void read_elements(std::vector<Tome> *elements, nlohmann::json const &j,
                   Schema const &s, int dim, std::vector<int64_t> &shape)
{
    if (dim == (int)shape.size())
    {
        if (elements)
        {
            elements->emplace_back();
            read_json(&elements->back(), j, s);
        }
        else
        {
            read_json(nullptr, j, s);
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

void read_json(Tome *tome, nlohmann::json const &j, ArraySchema const &s)
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

void read_json(Tome *tome, nlohmann::json const &j, DictSchema const &s)
{
    if (!j.is_object())
        throw ValidationError("expected object");
    if (tome)
        *tome = Tome::dict();

    auto found = std::vector<bool>(s.items.size(), false);

    // 1) check all items in the json object against the scheme and read
    // them
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

void write_json(nlohmann::json &, Tome const &, NoneSchema const &)
{
    throw ValidationError("NoneSchema is never valid");
}

void write_json(nlohmann::json &j, Tome const &tome, AnySchema const &)
{
    if (tome.is_boolean())
        j = tome.as_boolean();
    else if (tome.is_integer())
        j = tome.as_integer();
    else if (tome.is_real())
        j = tome.as_real();
    else if (tome.is_complex())
        j = {tome.as_complex().real(), tome.as_complex().imag()};
    else if (tome.is_string())
        j = tome.as_string();
    else
        throw WriteError("unsupported type when writing AnySchema to JSON");
}

void write_json(nlohmann::json &j, Tome const &tome, BooleanSchema const &)
{
    if (!tome.is_boolean())
        throw ValidationError("expected boolean");
    j = tome.as_boolean();
}

void write_json(nlohmann::json &j, Tome const &tome, NumberSchema const &)
{
    if (tome.is_integer())
        j = tome.as_integer();
    else if (tome.is_real())
        j = tome.as_real();
    else if (tome.is_complex())
        j = {tome.as_complex().real(), tome.as_complex().imag()};
    else
        throw ValidationError("expected number");
}

void write_json(nlohmann::json &j, Tome const &tome, StringSchema const &)
{
    j = tome.as_string();
}

void write_elements(nlohmann::json &j, Tome const *&elements, Schema const &s,
                    int dim, std::vector<size_t> &shape)
{
    if (dim == (int)shape.size())
    {
        write_json(j, *elements++, s);
        return;
    }

    j = nlohmann::json::array();

    for (size_t i = 0; i < shape[dim]; ++i)
    {
        j.push_back(nullptr);
        write_elements(j[i], elements, s, dim + 1, shape);
    }
}

void write_json(nlohmann::json &j, Tome const &tome, ArraySchema const &s)
{
    if (!tome.is_array())
        throw ValidationError("expected array");
    auto const &a = tome.as_array();
    auto shape = a.shape();

    auto it = &a.flat()[0];
    write_elements(j, it, s.elements, 0, shape);
}

void write_json(nlohmann::json &j, Tome const &tome, DictSchema const &s)
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

        write_json(j[item.key], it->second, item.schema);
    }
}
} // namespace
} // namespace scribe

void scribe::read_json(Tome *tome, nlohmann::json const &j, Schema const &s)
{
    s.visit([&](auto const &s) { read_json(tome, j, s); });
}

void scribe::write_json(nlohmann::json &j, Tome const &tome, Schema const &s)
{
    s.visit([&](auto const &s) { write_json(j, tome, s); });
}

void scribe::validate_json_file(std::string_view filename, Schema const &s)
{
    auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                   nullptr, true, true);
    read_json(nullptr, j, s);
}
