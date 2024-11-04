#pragma once

// Basic definitions that are used by most other header files in Scribe.
// Mostly typedefs and concepts. Also error classes.

#include "xtensor/xarray.hpp"
#include <stdexcept>

namespace scribe {

// forward declarations
class JsonReader;
class Hdf5Reader;

struct ScribeError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// thrown when a Tome is not of the expected type
struct TomeTypeError : ScribeError
{
    using ScribeError::ScribeError;
};

// thrown when a data file/object does not follow the expected schema
struct ValidationError : ScribeError
{
    using ScribeError::ScribeError;
};

// thrown when a data file/object cannot be read
struct ReadError : ScribeError
{
    using ScribeError::ScribeError;
};

// thrown when a data file/object cannot be written
struct WriteError : ScribeError
{
    using ScribeError::ScribeError;
};

// helper for std::visit
template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// another helper that could be part of the standard library
template <class T, class... Ts>
concept OneOf = (std::same_as<T, Ts> || ...);

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

// simple scope guard
template <class F> struct ScopeGuard
{
    F f;
    ScopeGuard(F f) : f(f) {}
    ~ScopeGuard() { f(); }
    ScopeGuard(ScopeGuard const &) = delete;
    ScopeGuard &operator=(ScopeGuard const &) = delete;
};

#define SCRIBE_DEFER(code)                                                     \
    scribe::ScopeGuard _deferred_##__LINE__                                    \
    {                                                                          \
        [&] { code; }                                                          \
    }

template <class R>
concept Reader = requires(R &r, std::string_view key, bool &b, std::string &s,
                          int &i, double &d, std::complex<double> &c) {
    r.push(key);
    r.pop();
    r.read(b, key);
    r.read(s, key);
    r.read(i, key);
    r.read(d, key);
    r.read(c, key);
};

} // namespace scribe