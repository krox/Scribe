# The `Tome` type

The `scribe::Tome` type is a run-time-generic container for arbitrary data, similar to the `nlohmann::json` type. Example:

```C++
Tome x = int32_t(5);
Tome y = "hello world";
fmt::print("{} {}", x, y);
```

Any `Tome` object contains exactly one of the following types:
* Atomic types:
  * `bool`
  * `std::string`
  * integers: `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
  * floating point numbers: `float`, `double`
  * complex numbers: `std::complex<float>`, `std::complex<double>`
* Compound Types:
  * `Tome::dict_type`, which is a map `string`->`Tome`
  * `Tome::array_type`, which is an (arbitrary dimensional) array of `Tome` values
  * `Tome::numeric_array_type<T>`, where `T`is one of the integer/floating/complex types. This is a memory-optimized version of `array_type` that only holds one type of elements.

## Creating a `Tome` from data
1) The templated constructor `Tome(auto&& data)` does its best to determine the appropriate type from data.

2) To be more explicit about the resulting type you can use the static member functions
* `Tome::integer(...)`
* `Tome::real(...)`
* `Tome::complex(...)`
* `Tome::array(...)`
* `Tome::dict(...)`
* `Tome::numeric_array(...)`

## Getting data out of a `Tome`

There are many ways to get data out of a `Tome` object again. Roughly ordered by complexity, they are:
### Operations directly on `Tome` object:
```C++
Tome x = ...;

Tome elem = x["foo"]; // will throw if x is not a dict
Tome elem = x[0]; // will throw if x is not an array

fmt::print("{}", x);
scribe::write_file(filename, x, schema);
```

### Using the `.as<T>()` method:
This returns a reference. I.e., it does not cause a copy and allows direct writes to the contained data (unless the Tome is `const` of course)
```C++
Tome x = 3;
x.as<int> = 5;
int y = x.as<int>() * 2; // type must match exactly
assert(y == 10);
```
Note however that the type must match exactly, otherwise a `scribe::TomeTypeError` is thrown.

### Using the `.get<T>()` method:
This returns a copy of the contained data, but is more flexible with types. In particular it can be customized to make `tome.get<MyType>()` work. See details on the `TomeSerializer` class below.

### Using the visitor pattern

If the exacty type of the data is not known, the `.visit` member function might be useful, in particular in conjunction with the `overloaded` helper and some C++20 concepts:
```C++
Tome x = ...;
x.visit(scribe::overloaded{
    [](float val){ fmt::print("got a float: {}", val); },
    [](scribe::IntegerType auto val){ fmt::print("got an integer (any width/signdness): {}", val); },
    [](auto const&){ fmt::print("some other type"); }
});
```
The provided concepts are:
```
scribe::IntegerType:
    int8_t, int16_t, int32_t, int64_t,
    uint8_t, uint16_t, uint32_t, uint64_t
scribe::RealType:
    float, double
scribe::ComplexType:
    std::complex<float>, std::complex<double>
scribe::NumberType:
    any of Integer/Real/Complex
scribe::AtomicType:
    any of NumberType, or bool, or std::string
scribe::NumericArrayType:
    homogeneous array containing any of the NumberType's
```

### Explicitly checking the types

The `.is<T>()` method can be used to verify the exact type.
```C++
Tome x = 42;
assert(x.is<int>());

x = ...;
if(x.is<int32_t>()) { use(x.as<int32_t>()); }
else if (x.is<int64_t>()) { use(x.as<int64_t>()); }
else ...
```
Additionally, we provide the methods `.is_integer()`, `.is_real()`, `.is_complex()`, `.is_number()`, `.is_atomic()`, `.is_numeric_array()` for convenience, that mirror the concepts shown above.

## Numerical types

In contrast to `nlohmann::json`, a `Tome` precisely tracks the type of any contained numbers and does not implicitly convert between different sizes/precisions. Also complex numbers are supported directly.
Examples:

```C++
Tome x = int32_t(5);
Tome y = int64_t(6);
Tome z = std::complex<float>(42.0, 23.0);

assert(x.is<int32_t>() && !x.is<int64_t>());
assert(!y.is<int32_t>() && y.is<int64_t>());
assert(x.is_integer() && y.is_integer());
assert(z.is_complex() && z.is<std::complex<float>>());

int64_t value = y.get<int64_t>(); // retrieve the value again
//y.get<int32_t>(); // will throw a scribe::TomeTypeError
```

## Dictionaries

Dicts work as one would expect. In particular, a default-constructed `Tome` will be an (empty) dictionary. as there is no dedicated `null` type.
```C++
Tome x; // x is now an empty dict.
//Tome x = Tome::dict(); // same thing more explicitly
x["foo"] = 42;
y["bar"]["baz"] = 23; // arbitrary nesting of dicts
```

## Standard Arrays

1D arrays behave just one would expect, roughly mirroring the interface of a `std::vector<Tome>`:
```C++
// 1D array containing arbitrary types
Tome x = Tome::array(); // empty 1D array
x.push_back("foo");
x.push_back(42);
x[0] = "bar";
assert(x.size() == 2);
for(auto const& elem: x)
    fmt::print("{}", elem);
```

Multi-dimensional arrays are straight-forward as well, but do not support any `.push_back` of course:

```C++
Tome x = Tome::array_from_shape({10,10});
for(int i = 0; i < 10; ++i)
    for(int j = 0; j < 10; ++j)
        x(i,j) = i+j;
assert(x.rank() == 2);
assert(x.shape()[0] == 10);
assert(x.shape()[1] == 10);
```

## Numerical Arrays

These are arrays containing homogeneous numerical data, i.e., one of the `NumberType`'s defined above. In memory, they are stored densely, without the indirection of a `Tome` object for every element. While this is a lot more efficient than a standard array, the interface can be a bit more awkward.

```C++
Tome x = Tome::numeric_array<double>({10, 10}); // shape=(10,10), data uninitialized

// x[...] = ...; // not supported.

// instead have to use:
auto& values = x.as_numeric_array<double>();
for(int i = 0; i < 10; ++i)
    for(int j = 0; j < 10; ++j)
        values(i,j) = i*j;
```

NOTE:
* From a python perspective, a standard array corresponds to a `list`, and a numerical array to a `numpy.ndarray`.
* Under the hood, arrays are implemented using the `xtensor` library. Thus the `.as_numeric_array` function returns some version of a `xt::xarray` type.


## Converting user-defined types to/from `Tome`

Conversion of arbitrary types to/from `Tome` can be achieved by specializing the `TomeSerializer` class. This is the same pattern as can be found in nlohmann's json library for example:

```C++
struct Point
{
    float x,y;
};

template <> struct scribe::TomeSerializer<bool>
{
    static Tome to_tome(Point const& value)
    {
        Tome r;
        r["x"] = value.x;
        r["y"] = value.y;
        return r;
    }

    static Point from_tome(Tome const &tome)
    {
        Point r;
        r.x = tome["x"].get<float>();
        r.y = tome["y"].get<float>();
        return r;
    }
};

int main()
{
    Tome a = Point(1,2);      // Point -> Tome
    Point b = a.get<Point>(); // Tome -> Point
}

```

Of course, this is a rather pedestrian approach to serialization. Generated code based on a Schema would make above code obsolete.