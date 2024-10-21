#pragma once

#include "fmt/format.h"
#include "scribe/base.h"
#include "scribe/schema.h"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
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

// all the atomic types a Tome can hold
using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;
using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;
using float32_t = float;
using float64_t = double;
using complex_float32_t = std::complex<float>;
using complex_float64_t = std::complex<double>;
using string_t = std::string;
using bool_t = bool;

// concepts for nice template constraints
template <class T>
concept IntegerType = OneOf<T, int8_t, int16_t, int32_t, int64_t, uint8_t,
                            uint16_t, uint32_t, uint64_t>;
template <class T>
concept RealType = OneOf<T, float32_t, float64_t>;
template <class T>
concept ComplexType = OneOf<T, complex_float32_t, complex_float64_t>;
template <class T>
concept NumberType = IntegerType<T> || RealType<T> || ComplexType<T>;
template <class T>
concept AtomicType = IntegerType<T> || RealType<T> || ComplexType<T> ||
                     std::same_as<T, bool_t> || std::same_as<T, string_t>;

template <class T> struct TomeSerializer;

class Tome
{
  public:
    using dict_type = std::map<std::string, Tome>;
    using array_type =
        xt::xarray_container<std::vector<Tome>, xt::layout_type::row_major,
                             std::vector<size_t>>;

    // NOTE: 'dict_type' should be first, as it is the default for 'Tome'
    using variant_type =
        std::variant<dict_type, array_type, string_t, bool_t, int8_t, int16_t,
                     int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t,
                     float32_t, float64_t, complex_float32_t,
                     complex_float64_t>;

    template <class R = void, class Visitor> R visit(Visitor &&vis)
    {
        return std::visit<R>(std::forward<Visitor>(vis), data_);
    }
    template <class R = void, class Visitor> R visit(Visitor &&vis) const
    {
        return std::visit<R>(std::forward<Visitor>(vis), data_);
    }

  private:
    variant_type data_;

    // private backend constructor
    struct direct
    {};
    Tome(direct, variant_type &&data) : data_(std::move(data)) {}

  public:
    // get contained data. Throws on type mismatch
    // TODO: the 'as'/'is' macros need template constraints
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

    // check type
    template <class T> bool is() const
    {
        return std::holds_alternative<T>(data_);
    }

    // convenience-wrappers of the above to save some typing.
    bool_t &as_boolean() { return as<bool_t>(); }
    string_t &as_string() { return as<string_t>(); }
    array_type &as_array() { return as<array_type>(); }
    dict_type &as_dict() { return as<dict_type>(); }
    bool_t as_boolean() const { return as<bool_t>(); }
    string_t const &as_string() const { return as<string_t>(); }
    array_type const &as_array() const { return as<array_type>(); }
    dict_type const &as_dict() const { return as<dict_type>(); }
    bool is_boolean() const { return is<bool_t>(); }
    bool is_string() const { return is<string_t>(); }
    bool is_array() const { return is<array_type>(); }
    bool is_dict() const { return is<dict_type>(); }

    // true for int8, int16, int32, int64, uint8, uint16, uint32, uint64
    bool is_integer() const
    {
        return is<int8_t>() || is<int16_t>() || is<int32_t>() ||
               is<int64_t>() || is<uint8_t>() || is<uint16_t>() ||
               is<uint32_t>() || is<uint64_t>();
    }

    // true for float32, float64
    bool is_real() const { return is<float32_t>() || is<float64_t>(); }

    // true for complex_float32, complex_float64
    bool is_complex() const
    {
        return is<complex_float32_t>() || is<complex_float64_t>();
    }

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
    // NOTE: these should typically be used when implementing the
    // `TomeSerializer` trait for custom types
    static Tome boolean(bool_t value) { return Tome(direct{}, bool_t{value}); }
    template <IntegerType T> static Tome integer(T value)
    {
        return Tome(direct{}, value);
    }
    template <RealType T> static Tome real(T value)
    {
        return Tome(direct{}, value);
    }
    template <ComplexType T> static Tome complex(T value)
    {
        return Tome(direct{}, value);
    }
    static Tome string(std::string_view value)
    {
        return Tome(direct{}, string_t{value});
    }
    static Tome array() { return Tome(direct{}, array_type{}); }
    static Tome array(array_type const &a) { return Tome(direct{}, a); }
    static Tome array(array_type &&a) { return Tome(direct{}, std::move(a)); }

    static Tome array(std::vector<Tome> elems, std::vector<size_t> shape)
    {
        // auto r = array_type::from_shape(shape);
        std::vector<ptrdiff_t> strides(shape.size());
        auto size =
            xt::compute_strides(shape, xt::layout_type::row_major, strides);
        if (size != elems.size())
            throw std::runtime_error("size mismatch");
        auto data =
            array_type(std::move(elems), std::move(shape), std::move(strides));
        return Tome(direct{}, std::move(data));
    }
    static Tome array(std::vector<double> data, std::vector<size_t> shape)
    {
        // TODO: compact representation goes here
        std::vector<Tome> elems;
        elems.reserve(data.size());
        for (auto const &d : data)
            elems.push_back(Tome::real(d));
        return array(array_type(xt::adapt(elems, shape)));
    }
    static Tome array(std::vector<int64_t> data, std::vector<size_t> shape)
    {
        std::vector<Tome> elems;
        elems.reserve(data.size());
        for (auto const &d : data)
            elems.push_back(Tome::integer(d));
        return array(array_type(xt::adapt(elems, shape)));
    }
    static Tome dict() { return Tome(direct{}, dict_type{}); }

    // construct a Tome of type 'type', simply 'static_cast'ing the value. I.e.
    // no range checks. Used as backend helper, typically after validation.
    static Tome number_unchecked(auto value, NumType type)
    {
        switch (type)
        {
        case NumType::INT8:
            return integer(static_cast<int8_t>(value));
        case NumType::INT16:
            return integer(static_cast<int16_t>(value));
        case NumType::INT32:
            return integer(static_cast<int32_t>(value));
        case NumType::INT64:
            return integer(static_cast<int64_t>(value));
        case NumType::UINT8:
            return integer(static_cast<uint8_t>(value));
        case NumType::UINT16:
            return integer(static_cast<uint16_t>(value));
        case NumType::UINT32:
            return integer(static_cast<uint32_t>(value));
        case NumType::UINT64:
            return integer(static_cast<uint64_t>(value));
        case NumType::FLOAT32:
            return real(static_cast<float32_t>(value));
        case NumType::FLOAT64:
            return real(static_cast<float64_t>(value));
        case NumType::COMPLEX_FLOAT32:
            return complex(static_cast<complex_float32_t>(value));
        case NumType::COMPLEX_FLOAT64:
            return complex(static_cast<complex_float64_t>(value));
        default:
            throw std::runtime_error("invalid NumType");
        }
    }

    // Conversion from any type
    // NOTE: implement for custom types by specializing `TomeSerializer`
    template <class T> Tome(T const &value)
    {
        *this = TomeSerializer<T>::to_tome(value);
    }

    // Conversion to any type
    // NOTE: implement for custom types by specializing `TomeSerializer`
    // NOTE: for standard types, some conversions are implicit (floating point
    // precision change, widening integer conversions), but unsafe stuff is not
    // (e.g. narrowing integer conversions)
    template <class T> T get() const
    {
        return TomeSerializer<T>::from_tome(*this);
    }

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
    std::vector<size_t> shape() const
    {
        return {as_array().shape().begin(), as_array().shape().end()};
    }
    Tome &operator[](size_t i) { return as_array()(i); }
    Tome const &operator[](size_t i) const { return as_array()(i); }

    /*Tome &operator[](std::span<const size_t> i) { return as_array().at(i); }
    Tome const &operator[](std::span<const size_t> i) const
    {
        return as_array().at(i);
    }*/
};

// read/write a tome from/to a file. File format is determined by suffix
void read_file(Tome &, std::string_view filename, Schema const &);
void write_file(std::string_view filename, Tome const &, Schema const &);

// read/write a tome from/to a JSON string
void read_json_string(Tome &, std::string_view json, Schema const &);
void write_json_string(std::string &json, Tome const &, Schema const &);

// throws ValidationError if the file does not follow the schema
void validate_file(std::string_view filename, Schema const &s);

// Guess the schema from data.
//   * this should be considered unstable because there is some guess-work
//     involved (especially if the data came from a weakly-typed source such as
//     JSON.)
//   * mostly useful for interactive exploration of data files, or as a starting
//     point for creating a schema for already existing data files with unknown
//     schema.
Schema guess_schema(Tome const &);

template <> struct TomeSerializer<bool>
{
    static Tome to_tome(bool value) { return Tome::boolean(value); }
    static bool from_tome(Tome const &tome) { return tome.as_boolean(); }
};

template <IntegerType T> struct TomeSerializer<T>
{
    static Tome to_tome(T value) { return Tome::integer(value); }
    static T from_tome(Tome const &tome)
    {
        // NOTE: implicit conversions like int8->int16 could go here
        return tome.as<T>();
    }
};

template <RealType T> struct TomeSerializer<T>
{
    static Tome to_tome(T value) { return Tome::real(value); }
    static T from_tome(Tome const &tome)
    {
        // NOTE: implicit conversions like float32->float64 could go here
        return tome.as<T>();
    }
};

template <ComplexType T> struct TomeSerializer<T>
{
    static Tome to_tome(T value) { return Tome::complex(value); }
    static T from_tome(Tome const &tome)
    {
        // NOTE: implicit conversions like complex_float32->complex_float64
        // could go here
        return tome.as<T>();
    }
};

/*
template <class T> struct TomeSerializer<std::vector<T>>
{
    static Tome to_tome(std::vector<T> const &value)
    {
        return Tome::array(value, {value.size()});
    }
    static T from_tome(Tome const &tome)
    {
        std::vector<T> result;
        auto const &a = tome.as_array();
        if (a.rank != 1)
            throw TomeTypeError("expected a 1D array");
        result.reserve(a.size());
        for (T const &v : a)
            result.push_back(TomeSerializer<T>::from_tome(v));
        return result;
    }
};
*/

template <> struct TomeSerializer<std::string>
{
    static Tome to_tome(std::string_view value) { return Tome::string(value); }
    static std::string const &from_tome(Tome const &tome)
    {
        return tome.as_string();
    }
};

} // namespace scribe