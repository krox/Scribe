#pragma once

#include <stdexcept>

namespace scribe {
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
} // namespace scribe