# Design Document for Scribe

This document is essentially a summary of my thoughts and plans, as well as the result of multiple discussions in our group on goals, non-goals and general requirements of the new serialization library.

## Scope

Scribe is a general-purpose serialization library with a focus on large numerical datasets.
* `Scribe` does not specify an on-disk file formats. Instead it can use a selection of well-known formats like `.json` and `.hdf5`.
* `Scribe` specifies data-schemas that describe the layout of the data. This schema is independent of the file-format, thus it can in priniciple be used to convert between file-formats (though this is not the main focus of the library). A simple schema might look like this:
  ```json5
  {
    "schema_name": "MyData",
    "schema_description": "A longer, human description goes here",

    // most schemas are expected to be dictionaries at top level
    "type": "dict",
    "items":
    [
        {
            "key": "foo",
            "type": "int32",
        },
        {
            "key": "bar",
            "optional":true,
            "type": "array",
            "rank": 1,
            "elements": {
                "type": "float64"
            }      
        }
    ]
  }
  ```
* `Scribe` can be used in multiple modes:
  * As a command line tool. For example
    ```bash
    scribe validate schema.json data.hdf5
    ```
    checks whether a data-file adheres to a given schema
  * As a C++ library:
    ```C++
    #include "scribe/scribe.h"

    int main()
    {
        auto schema = Scribe::Schema("my_schema.json");
        Scribe::Tome data = Scribe::read("data.hdf5", schema);

        fmt::print("{}\n", data["foo"].get<int>());
    }
    ```
    Note here that `Scribe::Tome` is a generic runtime data-object which can hold arbitrary amounts of nested data, similar to `nlohmann::json`.
  * As a C++ code generator. Calling `scribe codegen-cpp myschema.json` will generate a header file
    ```C++
    struct MyData
    {
        int32_t foo;
        std::vector<double> bar;

        static MyData read(std::string_view filename);
        void write(std::string_view filename);
    };
    ```
    as well as implementations for the read/write functions.

## Similar projects / why this cannot be done with X

### Scribe is not json-schema
* `json-schema` is purely for validation, whereas `Scribe` also does data read/write
* `json-schema` can only describe json data, not more low-level formats. For example it cannot reasonably distinguish between single- and double-precision floating point numbers.
* `json-schema` lacks an efficient understanding of multi-dimensional homogeneous numerical arrays, which are crucial to our usecases in scientific computations.

### Scribe is not ProtoBuffers
* `ProtoBuffers` are mainly concerned with reading and writing its own (platform-independent, but otherwise opaque) binary format. Scribe is concerned with describing existing data in well-known formats like `hdf5` and `json`
* while large numerical arrays can in principle be stored efficiently in a ProtoBuffer (using the `repeated` keyword), multi-dimensional arrays are not easy to implement. Scribe aims for an interface that should feel very natural to experienced users of `hdf5` or numpy's `ndarray`.

## The Schema format

Every schema has the fields:
* `type`, which must be either
  * one of the builtin types `boolean`, `string`, `int{8,16,32,64}`, `uint{8,16,32,64}`, `complex{64,128}`, `array`, `dict`, `variant`
  * one of the place-holders `none` (which is a schema that always rejects), or `any` (which accepts arbitrary data). 
  * a sub-schema, identified by name, prefixed by `$`
  * if `type` is not present, it defaults to `any`
* Optional meta-data
  * `schema_name`: This can be used to identify the schema for re-use. 
  * `schema_description`: human-readable description without any specific semantics
  * TBD: additional meta-data like "example", "default", "author", "version" are possible. Needs more discussion, as some of these could carry semantic meaning, while others dont.
* Nested types (`array`, `dict`, and `variant`) contain a schema that describes each of it constituents.
* Optionally, additional constraints that narrow the range of allowed values (e.g. min/max values for numbers, shape/size of arrays).
* Optioanlly, file-format specific hints on storage. E.g., `chunk_size` for hdf5 files.
  
### Numeric types

Numeric types are identified by `"type"` being one of `int{8,16,32,64}`, `uint{8,16,32,64}`, `complex{64,128}`. Possible constraints are
* `min_value` and `max_value` that specify the allowed range of values

Example:
```json
{
    "type": "int32",
    "min_value": 3
    "max_value": 8
}
```

### String type
Example:
```json
{
    "schema_name": "username",
    "type": "string",
    "min_size": 3,
    "max_size": 16
}
```

### Array type
Example 1: one-dimensional array of real numbers:
```json
{
    "schema_name": "correlator",
    "type": "array",
    "rank": 1,
    "elements":{
        "type": "float64"
    }
}
```

Example 2: large numerical array:
```json
{
    "schema_name": "propagator",
    "type": "array",
    "shape": [-1,-1,-1,-1, 4, 3],
    "hdf5":
    {
        "chunk_size": [4,4,4,4, 4,3],
        "fletcher32": true
    }
}
```
Note that `-1` has a special meaning. In `shape` it means this dimension can have arbitrary size (similar to specifying extents in the `Eigen` library, or C++'s `std::[md]span`). in `chunk_size`, it means the full dimensions should occopy a single chunk (simiar to the meaning of `-1` in NumPy's `.reshape` function)

### Dict type
In addition to `"type": "dict"`, the only required field is `items`, which is a list of possible items. Example
```json
{
    "schema_name": "MyDict",
    "type": "dict",
    "items": [
        {
            "key": "foo",
            "type": "int32"
        },
        {
            "key": "bar",
            "type": "float32",
            "optional": true
        }
    ]
}
```
which in C++ would match to
```C++
struct MyDict
{
    int32_t foo;
    std::optional<float> bar;
};
```

Instead of `key`, an item can have a `key_pattern` which understands `*` as a universal placeholder. Example:
```json
{
    "type": "dict",
    "items": [
        { "key_pattern": "foo_*" }
    ]
}
```
allows any key staring with `foo_`. Note that in case of overlapping patterns, order matters, the first matching key will always be taken, regardless of further constraints. Together with the `none` type, this allows some advanced usages. For example
```json
{
    "type": "dict",
    "items":[
        { "key": "foo_2" },
        { "key_pattern": "foo_*", "type": "none"},
        { "key_pattern": "*" }
    ]
}
```
which requires a key `foo_2`, disallows all other keys starting with `foo_`, but allows any key not starting with `foo_`.


### Variant type
This is a schema that can be one of several sub-schemas.
```json
{
    "type": "variant",
    "variants": [
        {"type": "string"}
        {"type": "integer"}
    ]
}
```
Restriction: For now, we require all sub-variants to be distinguished by their `type` field, in order to keep validation simple. (Sidenote: arbitrary variants together with recursive schemas could make validation undecidable, or at least exponentially hard. I think.)

## Mapping types between environments

| Scribe schema                                                                 | C++ type                                                       | Json example             |
| ----------------------------------------------------------------------------- | -------------------------------------------------------------- | ------------------------ |
| `"type": "none"`                                                              | -                                                              | -                        |
| `"type": "any"`                                                               | `Scribe::Tome`                                                 | `"foo"`                  |
| `"type": "int32"`                                                             | `int32_t`                                                      | `42`                     |
| `"type": "float64"`                                                           | `double`                                                       | `3.7`                    |
| `"type": "complex128"`                                                        | `std::complex<double>`                                         | `[1.0,2.0]`              |
| `"type": "string"`                                                            | `std::string`                                                  | `"Hello World`           |
| `"type": "array"`<br>`"rank": 1`                                              | `std::vector<...>`                                             | `[0.2,3.4,4.0]`          |
| `"type": "array"`                                                             | `Scribe::ndarray<...>`                                         | `[[1,2],[3,4]]`          |
| `"type": "dict"`<br>`"items":[{"key":"foo"}, {"key":"bar", "optional":true}]` | `struct {Scribe::Tome foo; std::optional<Scribe::Tome> bar; }` | `{"foo":5, "bar":[1,2]}` |
| `"type":"variant"` <br>`"variants":[{"type":"string"}{"type":"int32"}]`       | `std::variant<std::string, int32_t>`                           | `"hello"`                |

## File-Format specific considerations

### JSON

* Strictly speaking, JSON does not distinguish between integers and floating point numbers. In Scribe, we use this convention:
  * Numbers with a period `.` or a scientific notation `e` cannot be integers (even if it is `5.0`).
  * Numbers with only digits (and sign) are integers if and only if they are in the domain. E.g.:
    * `1234` is not a valid `int8_t`, but it is an `int32_t`. Numbers above `2^64-1` cannot be stored currently.
  * Any number can be interpreted as `float32/64`, regardless of magnitude or number of digits. E.g.:
    * `42` is a valid `float32/64`, though its also a valid integer.
    * `1e10000` as a `float32` is valid, though its value will be `inf`
    * `3.141592653589793` as a `float32` is valid, though the value will be rounded.
* JSON does in principle allow duplicate keys, Scribe does not.
* JSON does leave it to the implementation whether the order of keys matters. In Scribe, the order of keys when reading a json file does not matter. The order of keys specified in a schema however does matter in some cases (e.g. overlapping `key_pattern`s). This is one reason why it is a *List* of keys, and ant a *Map* (like in json-schema).


### HDF5

* The top-level of an HDF5 file is always a "group" (in HDF5 lingo), which maps to a `dict` in Scribe. Therefore only schemas that are `dicts` on top-level can be read from / written to HDF5. (This is not strictly true, depending on how non-numerical arrays are mapped. To be clarified.)
* Additional storage hints can be specified in a field called `hdf5`. For the time being the usecases are:
  * specifying chunks and checksums for arrays:
    ```json
    {
        "type":"array",
        "rank": 2
        "hdf5": {"chunk_size": [16,-1], "fletcher32": true}
    }
    ```
  * Explicitly specifying what kind of hdf5 object a scribe object should map to
    ```json
    {
        "type": "array",
        "shape": [3],
        "hdf5": {"category": "attribute"}
    }
    ```
    Without this field, the mapping is automatic:
    | Scribe type                           | HDF5 category                  |
    | ------------------------------------- | ------------------------------ |
    | `string`, `bool`, single number       | attribute                      |
    | `array` with numeric element type     | dataset                        |
    | `array` with non-numeric element type | group (maybe multiple levels?) |
    | `dict`                                | group                          |
* In HDF5, attributes can be not only be attached to groups, but also to datasets, this is not supported in Scribe. (Rationale: there is no clean way to map this to JSON)
* In HDF5, one can have an attribute and a dataset sharing a name (I think?). Scribe does not support this.
* HDF5 support arbitrary user-defined types. In scribe, we only use exactly two, namely for `complex64` and `complex128`.
* Future hdf5-specific hints might include:
  * more filters (for example lossless storage for small integers)
  * compression
  
## Real-World examples from the Hadrons library

### Perambulators
Conceptually, a perambulator is 7 dimensional, but it is stored in multiple files, each containing a 6-dimensional dataset. Each file can be described as
```json
{
    "schema_name": "perambulator_slice",

    "type": "dict",

    "items":[
        {
            "key": "GridDimensions",
            "hdf5": {"category": "attribute"},
            "type": "array",
            "shape": [3],
            "elements": {"type": "uint64"}
        },
        {
          "key": "TensorDimensions",
            "hdf5": {"category": "attribute"},
            "type": "array",
            "shape": [6],
            "elements": {"type": "uint64"}
        },
        {
          "key": "_Grid_dataset_threshold",
            "hdf5": {"category": "attribute"},
            "type": "uint32",
        },
        {
            "key": "IndexNames",
            "type": "dict",
            "items":[
                {
                  "key_pattern": "IndexNames_*",
                  "hdf5": {"category": "attribute"},
                  "type": "string",
                },
                {
                  "key": "_Grid_vector_size",
                  "type": "$GridVectorSize"
                },
            ]
        },
        {
            "key": "MetaData",
            "type": "dict",
            "items": [
                {
                  "key": "Version",
                  "hdf5": {"category": "attribute"},
                  "type": "string",
                },
                {
                  "key": "timeDilutionIndex",
                  "hdf5": {"category": "attribute"},
                  "type": "int32",
                },
                {
                    "key": "noiseHashes",
                    "type": "dict",
                    "items":[
                        {
                          "key": "_Grid_vector_size",
                           "type": "$GridVectorSize"
                        },
                        {
                          "key_pattern": "noiseHashes_*",
                          "hdf5": {"category": "attribute"},
                          "type": "string"
                        }
                    ]
                }
            ]
        },
        {
            "key": "Perambulator",
            "type": "array",
            "rank": 6,
            "elements": {"type": "complex128"}
        }
    ],
    "defs":[
        {
            "schema_name": "GridVectorSize",
            "hdf5": {"category": "attribute"},
            "type": "uint64",
        }
    ]
}
```

## Minimum viable program / TODO list

This is a rough todo-list what we need to make this project slightly useful to some. The goal is to have something presentable as soon as possible in order to look for potential users outside our immediate science community. Also, this list should be transformed to somethink like a kanban board.

1) Write the `Schema` class, which itself can be read from (and written to?) a json file.
2) Write a schema for the schemas, in the form of a json-schema. Then find a validation library that can check it for any given schema.
3) Write the `Tome` class. This is similar to the `nlohmann::json` type. Also discuss whether we are okay calling is "Tome". Could go with something neutral like "Object" instead.
4) Write a validation function for `json` files. As a first pass, a function like
   ```C++
   bool validate_json(nlohmann::json const&, Schema const&);
   ```
   is sufficient, though an implementation based on nlohmann's SAX parser could be more efficient in case of big datasets.
5) Write a json reader
   ```C++
   Tome read_json(std::string_view filename, Schema const&);
   ```
   This should definitely be based on nlohmann's SAX parser, i.e. circumventing the `nlohmann::json` type itself.
6) HDF5 validation.
   ```C++
   bool validate_hdf5(std::string_view filename, Schema const&);
   ```
7) Of course this should be done without reading any actual data from the hdf5 file, only the meta-data.
8) HDF5 reader
   ```C++
   Tome read_hdf5(std::string_view filename, Schema const&);
   ```
9) C++ code generation. This is a huge item, should be done after the dynamic `Tome` based readers are done, because at that point, code generation is effectively just an optimization.
10) Tie most of it together with a command-line utility:
    ```bash
    scribe validate myschema.json # check that myschema.json is valid itself
    scribe validate mydata.hdf5 myschema.json # just calls the "validate_{hdf5,json}(...)" function
    scribe codegen myschem.json # C++ code generation
    ```

## Points for discussion

Minor points that could be discussed in a future meeting (alternatively, actually writing the code might make it clearer what to do):
* How strict should our reader be?
  * A missing (but non-optional) key could be silently interpreted as an empty array/dict, provided this adheres to all other constraints
  * Is a json `null` value the same as a missing key?
  * Should validation fail if the only "error" is a wrong chunk_size?
  * Proposal: have a strict and non-strict mode. Validation defaults to strict, reading to non-strict.
* What exactly is the default mapping to hdf5 in regards to attribute vs group vs dataset? Proposal:
  * dict-values that are single numbers, booleans, string are attributes (and not datasets)
  * arrays with non-numeric element types are stored in groups, one level per dimension.
* How are arrays mapped to C++ exactly? Proposal: depends on specified rank/shape:
  * `rank=1`, `shape=[N]`: `std::array<..., N>` if `N` is small, `std::vector` otherwise
  * `rank=1`, shape unspecified: `std::vector`
  * `rank` larger or unspecified: `Tome::Array` (which consists of a flattened `std::vector<T>` and a shape, always standard row-major layout)
* Actually, we should allow "C++" format specific hints, just like for hdf5. This way the scheme could decide if an array should be mapped to `std::vector`, `std::array`, or maybe even `Eigen::Matrix` and friends.
* Should we support any "non-local" constraints? For example:
  * A Hadrons output file might specify a `GridDimension` attribute, and also a Correlator, the size of which must match with the value of the attribute.
  * Proposal: No. Thats an endless rabbit hole of complexity.

## Possible future extension

No promises when these will be implementd (if ever), but we like to keep these ideas around to make sure our design stays compatible in case we need any of them later.

### Regex

There are at least two places where regular expressions would come up naturally:
* As constraint for string data:
  ```json
  {
    "type": "string"
    "regex": "[a-zA-Z]*"
  }
  ```
* as a generalization of `key_pattern` inside a `dict`. Example:
  ```json
  {
    "type": "dict",
    "items":[
        {
            "key_regex": "foo_[0-9]*"
        }
    ]
  }
  ```
#### Discussion:
Both of these should be straight-forward to implement, assuming we can find and decide on a good implementation of regular expressions. Ideally, such a regex-library would need to support C++ gode generation. Right now, this does not seem to be required for our usecases.

### Reading parts of files using internal paths

the `Scribe::read` function would aquire a third (optional) parameter
```C++
scribe::read(std::string_view filename, Scribe::Schema const& schema, std::string_view path)
```
* Note that the return value of this function no longer adheres to the given schema, but to a sub-schema thereof. So it would be natural to add a helper function
  ```C++
  scribe::project_schema(schema, path)
  ```
  That determines the schema resulting from a partial read, whithout looking at any particular data.
* Some possible kinds of path are:
  * Simple paths like `"/foo"`, which (assuming the top-level schema is a dict), simply returns one element of the dict. Similary `"/foo/bar"` for nested dicts.
  * Similar to "pathname expansion" in bash, there could be wildcards like
    * `"/*/bar"`
    * `"/component_{a,b}"`
    * `"/foo_{1..100}`
  * Similar to "fancy-indexing" in NumPy's `ndarray`s, arrays could be indexed as
    * `/mydata[:,5,2:-1]`
  
#### Discussion

The simplest case (only dict-keys, no wildcards) should be implemented before version 1.0 of Scribe. Anything more needs a lot more work defining all semantics precisely, so this was decided against for the time being.

### More file formats

* `XML`: Not a huge fan, but probably required for existing codebases. Should be easy enough to implement, provided we find a good open-source library, equivalent to `nlohmann::json`. The same might be the case for `YAML`.
* NumPy's `.npy` format is actually quite easy to implement (very little meta-data, followed by the binary data of a single `nparray`). Of course this can only reasonably store schemas of the form
  ```json
  {
    "type": "array"
    "elements": {"type": "float32/float64/..."}
  }
  ```
  and not anything nested. Still might be nice
* `SQLite` databases as a file format have some serious pros (extremely stable, memory efficient). Can we map our schemas to a relational database? Need to read up on SQL database schemas, which have been a thing since forever I think.

### Python bindings

We could think about three levels of python support:
1) Binding for the `Schema` class, which allows validation of data files.
2) Binding for the `Tome` class, which allows reading and writing of data files.
3) A proper "Python Codegen" that makes a Schema into a python class at runtime.

#### Discussion
1) Is completely reasonable.
2) Not clear what the advantage of this is compared to using existing libraries like `h5py`, which integrate with NumPy better than we could probably hope to.
3) This could be a cool project (and a great showcase of Python's capabilities), but would require maintaining a lot of actual python code. There this is judged to be out of scope for the time being.

### Schema-retrival via URL

In the world of JSON schemas, it is common for schemas to be canonically named using a web URL, at which the full schema is publicly visible. To facilitate this, we can do two things:
1) Add a `schema_url` metadata, which points to a (publicly visible) adress
2) Allow `"type": "https://.../some_schema.json"` to refer to sub-schemas. Effectively, the prefix `"https://"` distinguishes this from a builtin-type or locally-defined subscheme (which are prefixed with `"$"`).

#### Discussion
Not very hard to do, but in the interest of keeping dependencies at a minimum, this will not be part of the first version of the library.

### Multi-file dataset

Quite often in lattice QCD, a "single" measurement is written in multiple files. Very roughly, we want to do something like
```C++
scribe::read("propagator_t*.hdf5", schema);
```
where `schema` describes the full dataset, and not a single file. Some points of note:
* The schema should still not depend on the file-format, so logically, the same schema should be usable regardless of whether the data is in one file or multiple files.
* A more sane approach than the genral asterisk `*` might go something like this: Give each axis in a (multi-dimensional) array a name. In lattice QCD, these are typically `x,y,z,t,spin,color` and the like. Then the function call could be
  ```C++
  read("propagator_t{t}.hdf5", schema);
  ```
* Somewhat similar to the "fancy internal paths" from before, an individual file does no longer adhere to the given scheme, but to a projection thereof.

#### Discussion

Personally, I enjoy this direction of thought. it seems like we could draw some cool category-theory-style diagrams between `datasets`, `schemas` and `locations`, each with their own `project` function. But practically speaking, making all of this usable might be a multi-year research project in itself, so we reasonably decided against it.

Open question: Is there a well-defined special case of multi-file datasets that could be reasonably implemented on its own?

Idea: not multi-file, but splitting one logical nd-array along one axis along multiple datasets/groups inside a HDF5 file could be done quite reasonably inside the format-specific traits.


