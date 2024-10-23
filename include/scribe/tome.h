#pragma once

#include "fmt/format.h"
#include "fmt/ranges.h"
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

template <class T>
using Array = xt::xarray_container<std::vector<T>, xt::layout_type::row_major,
                                   std::vector<std::size_t>>;
class Tome;

// compact numerical arrays
using int8_array_t = Array<int8_t>;
using int16_array_t = Array<int16_t>;
using int32_array_t = Array<int32_t>;
using int64_array_t = Array<int64_t>;
using uint8_array_t = Array<uint8_t>;
using uint16_array_t = Array<uint16_t>;
using uint32_array_t = Array<uint32_t>;
using uint64_array_t = Array<uint64_t>;
using float32_array_t = Array<float32_t>;
using float64_array_t = Array<float64_t>;
using complex_float32_array_t = Array<complex_float32_t>;
using complex_float64_array_t = Array<complex_float64_t>;

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
template <class T>
concept NumericArrayType =
    OneOf<T, int8_array_t, int16_array_t, int32_array_t, int64_array_t,
          uint8_array_t, uint16_array_t, uint32_array_t, uint64_array_t,
          float32_array_t, float64_array_t, complex_float32_array_t,
          complex_float64_array_t>;
template <class T>
concept ArrayType = NumericArrayType<T> || std::same_as<T, Array<Tome>>;
template <class T>
concept CompoundType =
    std::same_as<T, Array<Tome>> ||
    std::same_as<T, std::map<std::string, Tome>> || NumericArrayType<T>;

template <class T>
concept TomeType = AtomicType<T> || CompoundType<T>;

template <class T> struct TomeSerializer;

class Tome
{
  public:
    using dict_type = std::map<std::string, Tome>;
    using array_type = Array<Tome>;

    // NOTE: 'dict_type' should be first, as it is the default for 'Tome'
    using variant_type =
        std::variant<dict_type, array_type, string_t, bool_t, int8_t, int16_t,
                     int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t,
                     float32_t, float64_t, complex_float32_t, complex_float64_t,
                     int8_array_t, int16_array_t, int32_array_t, int64_array_t,
                     uint8_array_t, uint16_array_t, uint32_array_t,
                     uint64_array_t, float32_array_t, float64_array_t,
                     complex_float32_array_t, complex_float64_array_t>;

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
    // default constructor creates an empty dict
    Tome() = default;

    // explicitly default copy/move constructor/assignment. This avoids
    // accidentally using the templated constructor (which would lead to weird
    // circular calls)
    Tome(Tome const &) = default;
    Tome(Tome &&) = default;
    Tome &operator=(Tome const &) = default;
    Tome &operator=(Tome &&) = default;

    // check contained type
    template <TomeType T> bool is() const
    {
        return std::holds_alternative<T>(data_);
    }
    bool is_boolean() const
    {
        return visit<bool>(overloaded{
            [](bool_t const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_string() const
    {
        return visit<bool>(overloaded{
            [](string_t const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_integer() const
    {
        return visit<bool>(overloaded{
            [](IntegerType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_real() const
    {
        return visit<bool>(overloaded{
            [](RealType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_complex() const
    {
        return visit<bool>(overloaded{
            [](ComplexType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_number() const
    {
        return visit<bool>(overloaded{
            [](NumberType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_atomic() const
    {
        return visit<bool>(overloaded{
            [](AtomicType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_standard_array() const
    {
        return visit<bool>(overloaded{
            [](array_type const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_numeric_array() const
    {
        return visit<bool>(overloaded{
            [](NumericArrayType auto const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_array() const { return is_standard_array() || is_numeric_array(); }
    bool is_dict() const
    {
        return visit<bool>(overloaded{
            [](dict_type const &) { return true; },
            [](auto const &) { return false; },
        });
    }
    bool is_compound() const { return is_array() || is_dict(); }

    // get contained value, throwing if the type is not as expected
    template <TomeType T> T &as()
    {
        if (auto *value = std::get_if<T>(&data_); value)
            return *value;
        throw TomeTypeError(
            fmt::format("Tome is not of type '{}'", typeid(T).name()));
    }
    template <TomeType T> T const &as() const
    {
        if (auto *value = std::get_if<T>(&data_); value)
            return *value;
        throw TomeTypeError(
            fmt::format("Tome is not of type '{}'", typeid(T).name()));
    }
    string_t &as_string() { return as<string_t>(); }
    string_t const &as_string() const { return as<string_t>(); }
    array_type &as_array() { return as<array_type>(); }
    array_type const &as_array() const { return as<array_type>(); }
    template <NumberType T> Array<T> &as_numeric_array()
    {
        return as<Array<T>>();
    }
    template <NumberType T> Array<T> const &as_numeric_array() const
    {
        return as<Array<T>>();
    }
    dict_type &as_dict() { return as<dict_type>(); }
    dict_type const &as_dict() const { return as<dict_type>(); }

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
    template <RealType T> static Tome complex(T re, T im)
    {
        return Tome(direct{}, std::complex<T>(re, im));
    }
    static Tome string(std::string_view value)
    {
        return Tome(direct{}, string_t{value});
    }

    static Tome dict() { return Tome(direct{}, dict_type{}); }
    static Tome dict(dict_type const &d) { return Tome(direct{}, d); }
    static Tome dict(dict_type &&d) { return Tome(direct{}, std::move(d)); }
    static Tome array() { return Tome(direct{}, array_type{}); }
    static Tome array(array_type const &a) { return Tome(direct{}, a); }
    static Tome array(array_type &&a) { return Tome(direct{}, std::move(a)); }
    template <NumericArrayType T> static Tome array(T a)
    {
        return Tome(direct{}, std::move(a));
    }

    // array from existing data. default to 1D if no shape is given
    static Tome array(std::vector<Tome> elems)
    {
        auto shape = std::vector<size_t>{elems.size()};
        return array(std::move(elems), std::move(shape));
    }
    static Tome array(std::vector<Tome> elems, std::vector<size_t> shape)
    {
        std::vector<ptrdiff_t> strides(shape.size());
        auto size =
            xt::compute_strides(shape, xt::layout_type::row_major, strides);
        if (size != elems.size())
            throw std::runtime_error(
                fmt::format("size mismatch (got {} elements, shape = ({}))",
                            elems.size(), fmt::join(shape, ", ")));
        auto data =
            array_type(std::move(elems), std::move(shape), std::move(strides));
        return Tome(direct{}, std::move(data));
    }
    template <NumberType T> static Tome array(std::vector<T> data)
    {
        auto shape = std::vector<size_t>{data.size()};
        return array(std::move(data), std::move(shape));
    }
    template <NumberType T>
    static Tome array(std::vector<T> data, std::vector<size_t> shape)
    {
        std::vector<ptrdiff_t> strides(shape.size());
        auto size =
            xt::compute_strides(shape, xt::layout_type::row_major, strides);
        if (size != data.size())
            throw std::runtime_error(
                fmt::format("size mismatch (got {} elements, shape = ({}))",
                            data.size(), fmt::join(shape, ", ")));
        return array(
            Array<T>(std::move(data), std::move(shape), std::move(strides)));
    }

    // array from shape, data uninitialized
    template <class T = Tome>
    static Tome array_from_shape(std::vector<size_t> shape)
    {
        return array(Array<T>::from_shape(shape));
    }

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
    size_t size() const
    {
        return visit<size_t>(overloaded{
            [](dict_type const &d) -> size_t { return d.size(); },
            [](array_type const &a) -> size_t { return a.size(); },
            [](NumericArrayType auto const &a) -> size_t { return a.size(); },
            [](auto const &) -> size_t {
                throw TomeTypeError("called '.size()' on a non-array/dict");
            }});
    }
    std::vector<size_t> shape() const
    {
        return visit<std::vector<size_t>>(overloaded{
            [](ArrayType auto const &a) -> std::vector<size_t> {
                return a.shape();
            },
            [](auto const &) -> std::vector<size_t> {
                throw TomeTypeError("called '.shape()' on a non-array");
            }});
    }

    size_t rank() const { return shape().size(); }

    // 1D array access
    Tome &operator[](size_t i) { return as_array()(i); }
    Tome const &operator[](size_t i) const { return as_array()(i); }

    template <class T> void push_back(T &&tome)
    {
        visit(overloaded{
            [&](array_type &a) {
                if (a.dimension() != 1)
                    throw TomeTypeError(
                        "called '.push_back()' on a non-1D array");

                // xarray does not support '.push_back' (and isnt supposed to).
                // but as our backend is actually a std::vector, we can just in
                // fact to it manually.
                auto &storage = a.storage();
                storage.push_back(std::forward<T>(tome));
                a.reshape({storage.size()});
            },
            [&](NumericArrayType auto const &) {
                throw TomeTypeError(
                    "called '.push_back()' on a non-standard array");
            },
            [&](auto const &) {
                throw TomeTypeError("called '.push_back()' on a non-array");
            }});
    }

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
    static bool from_tome(Tome const &tome) { return tome.as<bool>(); }
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

template <NumberType T> struct TomeSerializer<std::vector<T>>
{
    static Tome to_tome(std::vector<T> const &data)
    {
        return Tome::array(data, {data.size()});
    }
    static std::vector<T> from_tome(Tome const &tome)
    {
        auto const &a = tome.as_numeric_array<T>();
        if (a.dimension() != 1)
            throw TomeTypeError(
                "expected a 1D array (when converting Tome to std::vector)");
        return std::vector<T>(a.begin(), a.end());
    }
};

template <> struct TomeSerializer<std::string>
{
    static Tome to_tome(std::string_view value) { return Tome::string(value); }
    static std::string const &from_tome(Tome const &tome)
    {
        return tome.as_string();
    }
};

// string literals
template <size_t n> struct TomeSerializer<char[n]>
{
    static Tome to_tome(char const (&value)[n])
    {
        return Tome::string(std::string_view(value, n - 1));
    }
};

namespace detail {
template <class It, class T>
It format_arrray(It it, T const *&data, std::span<const size_t> shape,
                 size_t dim)
{
    if (dim == shape.size())
    {
        if constexpr (std::is_same_v<T, Tome>)
            return fmt::format_to(it, "{}", *data++);
        else if constexpr (scribe::ComplexType<T>)
        {
            it = fmt::format_to(it, "[{},{}]", data->real(), data->imag());
            ++data;
            return it;
        }
        else
            return fmt::format_to(it, "{}", *data++);
    }

    *it++ = '[';
    for (size_t i = 0; i < shape[dim]; i++)
    {
        if (i > 0)
            *it++ = ',';
        it = format_arrray(it, data, shape, dim + 1);
    }
    *it++ = ']';
    return it;
}
} // namespace detail

} // namespace scribe

template <> struct fmt::formatter<scribe::Tome>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(scribe::Tome const &tome,
                FormatContext &ctx) const -> decltype(ctx.out())
    {
        auto it = ctx.out();
        tome.visit(scribe::overloaded{
            [&](bool value) {
                it = fmt::format_to(it, "{}", value ? "true" : "false");
            },
            [&](std::string const &value) {
                it = fmt::format_to(it, "\"{}\"", value);
            },
            [&](scribe::IntegerType auto const &value) {
                it = fmt::format_to(it, "{}", value);
            },
            [&](scribe::RealType auto const &value) {
                it = fmt::format_to(it, "{}", value);
            },
            [&](scribe::ComplexType auto const &value) {
                it = fmt::format_to(it, "[{},{}]", value.real(), value.imag());
            },
            [&](scribe::Tome::dict_type const &value) {
                *it++ = '{';
                bool first = true;
                for (auto const &[key, val] : value)
                {
                    if (!first)
                        *it++ = ',';
                    first = false;
                    it = fmt::format_to(it, "\"{}\":{}", key, val);
                }
                *it++ = '}';
            },
            [&]<class T>(scribe::Array<T> const &value) {
                T const *data = value.storage().data();
                std::vector<size_t> const &shape = value.shape();
                it = scribe::detail::format_arrray(it, data, shape, 0);
            }});
        return it;
    }
};