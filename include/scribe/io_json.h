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

    friend void read(bool &value, JsonReader &file, std::string_view key)
    {
        if (!key.empty())
        {
            file.push(key);
            SCRIBE_DEFER(file.pop());
            read(value, file, {});
            return;
        }

        if (!file.current().is_boolean())
            throw ReadError("expected boolean at " + file.current_path());
        value = file.current().get<bool>();
    }

    friend void read(std::string &value, JsonReader &file, std::string_view key)
    {
        if (!key.empty())
        {
            file.push(key);
            SCRIBE_DEFER(file.pop());
            read(value, file, {});
            return;
        }

        if (!file.current().is_string())
            throw ReadError("expected string at " + file.current_path());
        value = file.current().get<std::string>();
    }

    template <IntegerType T>
    friend void read(T &value, JsonReader &file, std::string_view key)
    {
        if (!key.empty())
        {
            file.push(key);
            SCRIBE_DEFER(file.pop());
            read(value, file, {});
            return;
        }

        if (!file.current().is_number_integer())
            throw ReadError("expected integer at " + file.current_path());
        value = file.current().get<T>();
    }

    template <RealType T>
    friend void read(T &value, JsonReader &file, std::string_view key)
    {
        if (!key.empty())
        {
            file.push(key);
            SCRIBE_DEFER(file.pop());
            read(value, file, {});
            return;
        }

        if (!file.current().is_number())
            throw ReadError("expected floating point number at " +
                            file.current_path());
        value = file.current().get<T>();
    }

    friend void read(ComplexType auto &value, JsonReader &file,
                     std::string_view key)
    {
        if (!key.empty())
        {
            file.push(key);
            SCRIBE_DEFER(file.pop());
            read(value, file, {});
            return;
        }

        if (!file.current().is_array() || file.current().size() != 2 ||
            !file.current()[0].is_number() || !file.current()[1].is_number())
            throw ReadError("expected complex number at " +
                            file.current_path());
        auto real = file.current()[0].get<double>();
        auto imag = file.current()[1].get<double>();
        value = {real, imag};
    }

    template <class T>
    friend void read(std::optional<T> &value, JsonReader &reader,
                     std::string_view key)
    {
        if (key.empty())
        {
            if (!value)
                value.emplace();
            read(*value, reader, {});
        }
        else if (auto it = reader.current().find(key);
                 it != reader.current().end())
        {
            if (!value)
                value.emplace();
            read(*value, reader, key);
        }
        else
            value.reset();
    }

    template <class T>
    friend void read(std::vector<T> &value, JsonReader &reader,
                     std::string_view key)

    {
        if (!key.empty())
        {
            reader.push(key);
            SCRIBE_DEFER(reader.pop());
            return read(value, reader, {});
        }
    }
};

template <class R>
concept Reader = requires(R &r, std::string_view key, bool &b, std::string &s,
                          int &i, double &d, std::complex<double> &c) {
    r.push(key);
    r.pop();
    read(b, r, key);
    read(s, r, key);
    read(i, r, key);
    read(d, r, key);
    read(c, r, key);
};

static_assert(Reader<JsonReader>);

inline void read(auto &data, Reader auto &reader, std::string_view key)
{
    if (key.empty())
    {
        read(data, reader);
    }
    else
    {
        reader.push(key);
        SCRIBE_DEFER(reader.pop());
        read(data, reader);
    }
}

void read_file(auto &data, std::string_view filename)
{
    if (filename.ends_with(".json"))
    {
        auto file = scribe::JsonReader(filename);
        read(data, file);
    }
    else
        throw ReadError("dont recognize file format for " +
                        std::string(filename));
}

} // namespace scribe