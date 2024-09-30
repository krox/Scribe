
# The Schema format

A Schema is a json-object with the following fields
* `type`, which must be one of
  * atomic types: `"boolean"`, `"string"`, `"[u]int{8,16,32,64}"`, `"float{32,64}"`, `"complex{64,128}"`
  * compound types: `"array"`, `"dict"`
  * special types: `"any"`, `"none"`
* Optional meta-data
  * `schema_name`: only used in code-generation as name of the generated C++ class
  * `schema_description`
* For compound types, nested schema(s) describing their content
* Optionally, additional constraints that further restrict the allowed values (specific to types, see below)
  
## Numeric types

Number types (integer, float, complex) dont support any user-specified constraints (yet), though the implicit bounds of the type (e.g. $[0,255]$ for `uint8`) are validated.

## Special types

During validation, `any` always validates, `none` always fails.
When reading/writing data, this is generally not supported, but might be very useful when composing more complicated schemas.

## Strings

Optional `min_length` and `max_length` fields that constrain the size of the string.

Example:
```json
{
    "schema_name": "my_username",
    "type": "string",
    "min_length": 1,
    "max_length": 10,
}
```

## Array type

* Array schemas must have an `elements` key, which contains the schema that every element is validated against.
* Optionally, there can be a `shape` entry that specifies the dimensions of each axis. For examples `"shape": [3,3]` denotes a $3\times3$ matrix. As a special case, a dimension of `-1` leaves the size unspecified. I.e. `"shape":[-1]` specifies that the array is one-dimensional, but does not constrain the size.
* Though specifying a `shape` is optional (leaving the rank of the array arbitrary by default), some formats (i.e. JSON) do not offer a reliable way to determine the rank from the data file alon. In these cases, the `shape` entry is effectively required.

Example 1: one-dimensional array of real numbers:
```json
{
    "schema_name": "correlator",
    "type": "array",
    "shape": [-1],
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
    "elements": {
      "type": "complex128"
    }
}
```


### Dict type
Dict schemas must have an `items` field, which lists all valid keys. Additionally, `optional:true/false` can be used to mark an item as optional/required. By default, all elements are required.
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
* The list of items is specified as a json-array (not a json-object). This is a design choice to make implementing "key_pattern"s easier in the future.
* No duplicate key-names are allowed.
* If a data file contains keys that dont appear in the schema at all, validation will fail.

# Notes an mapping to specific file formats

While we strive for maximal generality, not every file format can store data from all valid schemas. Conversely, not every feature of every file format maps to a schema. We will mitigate this using format-specific storage hints, but to some degree, this is unavoidable.

## JSON

* Multi-Dimensional arrays are stored as nested arrays in "row-major" order (fastest moving index in the json file is the right-most axis of the array).
* Json `Null` values are not directly represented. In a JSON-object, a `"key":Null` entry is treated as a missing key (which might or might not be valid depending on the schema).
* complex numbers are represented as size-two arrays containing their real and imaginary part
* Strictly speaking, the JSON spec does not distinguish different numeric types (int vs float, or different length) at all, so Scribe has to make some choices when interpreting a json number (in line with most other libraries dealing with json data):
  * Integers may not include a decimal point (i.e. `42` is a valid integer, `42.0` is not)
  * Integers must fit into the specified type (i.e. `-1` is not a valid `uint32` and `128` is not a valid `int8`).
  * Any number is valid as a `float32` and `float64`. This includes
    * numbers without decimal point (e.g., `42` will be interpreted the same as `42.0`)
    * numbers outside representable range (e.g., `1.0e1000` will be interpreted as infinity and `1.0e-1000` as zero)
    * numbers with excessive precision (e.g., `3.141592653589793` will be silently rounded to `3.1415927` when used as `float32`)
* Strict JSON does not support comments at all. Scribe does "support" (i.e. ignore) comments of the `//` and `/* ... */` variety. This applies when reading schemas and when reading data from json files. Output from scribe will always adhere to strict JSON though, i.e., not contain any comments.
* Duplicate keys in a json objects are not supported and will always trigger a validation failure when reading (note that the JSON specification does discourage but not forbid duplicate keys and leaves the semantics up to implementation).
* When reading a json object, the order of keys does not matter. When writing a json object, the order of keys is unspecified, and might change seamingly random inbetween runs of the same code.
* Note that some of the above could be adjustable in the future using additional options in the schema (e.g. something like `"ignore_extra_keys":true`). In that case, the strict validation will remain the default though.


## HDF5

* The top-level type of schema must be `dict` in order to be storable in an hdf5 file.
* Dicts map to groups in HDF5 in the obvious way.
* By default, arrays where the elements have numeric type (integers, float, ...) are stored as datasets. 
* Arrays with other element-types are stored as groups containing keys `"0"`,`"1"`,`"2"`,... This can be multiple levels deep for multi-dimensional arrays.
* Numeric data (integers, floats, ...) that are not part of an array are stored as "scalar datasets" containing a single element.
* "Metadata" (in HDF5 lingo) is not supported. This will change in the future, but there are some design-decisions to be made before.
* Chunking and Fletcher32 checksums are turned on by default.
