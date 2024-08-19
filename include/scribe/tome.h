#pragma once

#include "fmt/format.h"
#include "scribe/array.h"
#include "scribe/schema.h"
#include <cassert>
#include <complex>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace scribe {

class TomeTypeError : public std::runtime_error
{
  public:
    TomeTypeError(std::string const &what) : std::runtime_error(what) {}
};

template <class T> struct TomeSerializer;

class Tome
{
  public:
    using boolean_type = bool;
    using integer_type = int64_t;
    using real_type = double;
    using complex_type = std::complex<double>;
    using string_type = std::string;
    using array_type = scribe::Array<Tome>;
    using dict_type = std::unordered_map<std::string, Tome>;

    using variant_type =
        std::variant<dict_type, boolean_type, integer_type, real_type,
                     complex_type, string_type, array_type>;

    template <class R = void, class Visitor> R visit(Visitor &&vis)
    {
        return std::visit(std::forward<Visitor>, data_);
    }
    template <class R = void, class Visitor> R visit(Visitor &&vis) const
    {
        return std::visit(std::forward<Visitor>(vis), data_);
    }

  private:
    variant_type data_;

    // private backend constructor
    struct direct
    {};
    Tome(direct, variant_type &&data) : data_(std::move(data)) {}

    template <class T> T &as()
    {
        if (auto *value = std::get_if<T>(&data_); value)
            return *value;
        throw TomeTypeError(
            fmt::format("Tome is not of type '{}'", typeid(T).name()));
    }

    template <class T> T const &as() const
    {
        if (auto *value = std::get_if<T>(&data_); value)
            return *value;
        throw TomeTypeError(
            fmt::format("Tome is not of type '{}'", typeid(T).name()));
    }

  public:
    // default constructor creates an empty dict
    Tome() = default;

    // explicitly default copy/move constructor/assignment. This avoids
    // accidentally using the templated constructor (which would lead to weird
    // circular calls)
    Tome(Tome const &) = default;
    Tome(Tome &&) = default;
    Tome &operator=(Tome const &) = default;
    Tome &operator=(Tome &&) = default;

    // pseudo-constructors with explicit types
    static Tome boolean(boolean_type value)
    {
        return Tome(direct{}, boolean_type{value});
    }
    static Tome integer(integer_type value)
    {
        return Tome(direct{}, integer_type{value});
    }
    static Tome real(real_type value)
    {
        return Tome(direct{}, real_type{value});
    }
    static Tome complex(complex_type value)
    {
        return Tome(direct{}, complex_type{value});
    }
    static Tome string(std::string_view value)
    {
        return Tome(direct{}, string_type{value});
    }
    static Tome array() { return Tome(direct{}, array_type{}); }
    static Tome array(std::vector<Tome> elements, std::vector<size_t> shape)
    {
        return Tome(direct{},
                    array_type{std::move(elements), std::move(shape)});
    }
    static Tome dict() { return Tome(direct{}, dict_type{}); }

    // get contained data. Throws on type mismatch
    boolean_type &as_boolean() { return as<boolean_type>(); }
    integer_type &as_integer() { return as<integer_type>(); }
    real_type &as_real() { return as<real_type>(); }
    complex_type &as_complex() { return as<complex_type>(); }
    string_type &as_string() { return as<string_type>(); }
    array_type &as_array() { return as<array_type>(); }
    dict_type &as_dict() { return as<dict_type>(); }

    // get contained data (const version)
    boolean_type as_boolean() const { return as<boolean_type>(); }
    integer_type as_integer() const { return as<integer_type>(); }
    real_type as_real() const { return as<real_type>(); }
    complex_type as_complex() const { return as<complex_type>(); }
    string_type const &as_string() const { return as<string_type>(); }
    array_type const &as_array() const { return as<array_type>(); }
    dict_type const &as_dict() const { return as<dict_type>(); }

    // conversion to/from arbitrary types
    template <class T> Tome(T const &value)
    {
        *this = TomeSerializer<T>::to_tome(value);
    }
    template <class T> T get() const
    {
        return TomeSerializer<T>::from_tome(*this);
    }

    // check type
    bool is_boolean() const
    {
        return std::holds_alternative<boolean_type>(data_);
    }
    bool is_integer() const
    {
        return std::holds_alternative<integer_type>(data_);
    }
    bool is_real() const { return std::holds_alternative<real_type>(data_); }
    bool is_complex() const
    {
        return std::holds_alternative<complex_type>(data_);
    }
    bool is_string() const
    {
        return std::holds_alternative<string_type>(data_);
    }
    bool is_array() const { return std::holds_alternative<array_type>(data_); }
    bool is_dict() const { return std::holds_alternative<dict_type>(data_); }

    // dict-like access
    Tome &operator[](std::string_view key)
    {
        return as_dict()[std::string(key)];
    }
    Tome const &operator[](std::string_view key) const
    {
        return as_dict().at(std::string(key));
    }

    // array-like access
    size_t size() const { return as_array().size(); }
    std::span<const size_t> shape() const { return as_array().shape(); }
    Tome &operator[](std::span<const size_t> i) { return as_array()[i]; }
    Tome const &operator[](std::span<const size_t> i) const
    {
        return as_array()[i];
    }

    // read from a file
    static Tome read_file(std::string_view filename, Schema const &schema);
    static Tome read_json_file(std::string_view filename, Schema const &schema);
    static Tome read_json_string(std::string_view json, Schema const &schema);

    // write to a file
    void write_file(std::string_view filename, Schema const &schema) const;
    void write_json_file(std::string_view filename, Schema const &schema) const;
};

template <> struct TomeSerializer<bool>
{
    static Tome to_tome(bool value) { return Tome::boolean(value); }
    static bool from_tome(Tome const &tome) { return tome.as_boolean(); }
};

template <std::integral T> struct TomeSerializer<T>
{
    static Tome to_tome(T value) { return Tome::integer(value); }
    static T from_tome(Tome const &tome) { return tome.as_integer(); }
};

template <> struct TomeSerializer<double>
{
    static Tome to_tome(double value) { return Tome::real(value); }
    static double from_tome(Tome const &tome) { return tome.as_real(); }
};
template <> struct TomeSerializer<std::string>
{
    static Tome to_tome(std::string_view value) { return Tome::string(value); }
    static std::string const &from_tome(Tome const &tome)
    {
        return tome.as_string();
    }
};

} // namespace scribe