#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/tome.h"
#include <fstream>

namespace scribe {
namespace internal {

// validates and reads a JSON object according to the given schema
//   * throws ValidationError if the JSON object does not follow the schema
//   * set tome=nullptr to only validate
void read_json(Tome *, nlohmann::json const &, Schema const &);

// writes a JSON object according to the given schema
void write_json(nlohmann::json &, Tome const &, Schema const &);

} // namespace internal

namespace internal {
std::vector<size_t> guess_array_shape(nlohmann::json const &json);

template <NumberType T>
void read_json_elements(typename Array<T>::iterator &it,
                        nlohmann::json const &j, std::span<const size_t> shape,
                        size_t dim)
{
    if (dim == shape.size())
    {
        if constexpr (ComplexType<T>)
        {
            if (!j.is_array() || j.size() != 2)
                throw ReadError("expected complex number");
            auto real = j[0].get<typename T::value_type>();
            auto imag = j[1].get<typename T::value_type>();
            *it++ = {real, imag};
            return;
        }
        else
        {
            *it++ = j.get<T>();
        }
        return;
    }

    if (!j.is_array() || j.size() != shape[dim])
        throw ReadError("inconsistent array shape");

    for (nlohmann::json const &elem : j)
        read_json_elements<T>(it, elem, shape, dim + 1);
}

template <NumberType T>
void read_json_array(Array<T> &value, nlohmann::json const &json)
{
    auto shape = internal::guess_array_shape(json);
    if constexpr (ComplexType<T>)
    {
        if (shape.empty() || shape.back() != 2)
            throw ReadError("expected complex array");
        shape.pop_back();
    }

    // Disallow dimension-zero arrays.
    if (shape.size() == 0)
        throw ReadError("expected array");

    value.resize(shape);
    auto it = value.begin();
    read_json_elements<T>(it, json, shape, 0);
}
} // namespace internal

class JsonReader
{
    using json = nlohmann::json;
    json json_;
    std::vector<std::reference_wrapper<const json>> stack_;
    std::vector<std::string> keys_;

    json const &current() const { return stack_.back().get(); }

  public:
    JsonReader(JsonReader const &) = delete;
    JsonReader &operator=(JsonReader const &) = delete;

    explicit JsonReader(std::string_view filename)
    {
        auto file = std::ifstream(std::string(filename));
        if (!file)
            throw ReadError("could not open file " + std::string(filename));
        file >> json_;
        stack_.push_back(std::cref(json_));
    }

    // human-readable location in the JSON file
    std::string current_path() const
    {
        return fmt::format("/{}", fmt::join(keys_, "/"));
    }

    void push(std::string_view key)
    {
        assert(!key.empty());
        if (!current().is_object())
            throw ReadError("expected object at " + current_path());
        auto it = current().find(key);
        if (it == current().end())
            throw ReadError("missing key '" + std::string(key) + "' at " +
                            current_path());
        keys_.push_back(std::string(key));
        stack_.push_back(std::cref(*it));
    }
    void pop() noexcept
    {
        assert(stack_.size() > 1);
        keys_.pop_back();
        stack_.pop_back();
    }

    void read(bool &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        if (!current().is_boolean())
            throw ReadError("expected boolean at " + current_path());
        value = current().get<bool>();
    }

    void read(std::string &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        if (!current().is_string())
            throw ReadError("expected string at " + current_path());
        value = current().get<std::string>();
    }

    template <IntegerType T> void read(T &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        if (!current().is_number_integer())
            throw ReadError("expected integer at " + current_path());
        value = current().get<T>();
    }

    template <RealType T> void read(T &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        if (!current().is_number())
            throw ReadError("expected floating point number at " +
                            current_path());
        value = current().get<T>();
    }

    void read(ComplexType auto &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        if (!current().is_array() || current().size() != 2 ||
            !current()[0].is_number() || !current()[1].is_number())
            throw ReadError("expected complex number at " + current_path());
        auto real = current()[0].get<double>();
        auto imag = current()[1].get<double>();
        value = {real, imag};
    }

    template <class T> void read(std::optional<T> &value, std::string_view key)
    {
        assert(!key.empty());
        if (!current().is_object())
            throw ReadError("expected object at " + current_path());

        if (current().contains(key))
        {
            if (!value)
                value.emplace();
            read(*value, key);
        }
        else
            value.reset();
    }

    template <NumberType T> void read(Array<T> &value, std::string_view key)
    {
        push(key);
        SCRIBE_DEFER(pop());

        internal::read_json_array(value, current());
    }
};

static_assert(Reader<JsonReader>);

} // namespace scribe