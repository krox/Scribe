
# The Schema format

Every schema has the fields:
* `type`, which must be one of
  * atomic types: `boolean`, `string`, `[u]int{8,16,32,64}`, `float{32,64}`, `complex{64,128}`
  * compound types: `array`, `dict`
  * special types: `any`, `none`
* Optional meta-data (not used for anything right now, but part of the library API)
  * `schema_name`
  * `schema_description`
* Additional information depending on the type
  
### Numeric types

Number types (integer, float, complex) dont support additional constraints right now, though the implicit bounds of the type (i.e. [0,255] for `uint8`) are validated.

## Special types

During validation, `any` always validates, `none` always fails.
When reading/writing data, this is generally not supported, but might be very useful when composing more complicated schemas.

## Strings

Optional `min_length` and `max_length` fields that constrain the size of the string.

Example:
```json
{
    "schema_name": "non_empty_string",
    "type": "string",
    "min_len": 1
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

Array schemas must have an `elements` key, which contains the schema that every element is validated against. Optionally, there can be a `shape` entry that specifies the dimensions of each axis. For examples `"shape": [3,3]` denotes a $3\times3$ matrix. As a special case, a dimension of `-1` leaves the size unspecified. I.e. `"shape":[-1]` specifies that the array is one-dimensional, but does not constrain the size.
* Though specifying a `shape` is optional, some formats (i.e. JSON) require at least a known rank, as this information can not be reliably determined from a data file alone.

Example 1: one-dimensional array of real numbers:
```json
{
    "schema_name": "correlator",
    "type": "array",
    "shape": [-1]
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

# Notes an mapping to specific file formats

While we strive for maximal generality, not every file format can store data from all valid schemas. Conversely, not every feature of every file format maps to a schema. We will mitigate this using format-specific storage hints, but to some degree, this is unavoidable.

## JSON

* Multi-Dimensional arrays are stored as nested arrays in "row-major" order (fastest moving index in the json file is the right-most axis of the array).
* Json `Null` values are not directly represented. In a JSON-object, a `"key":Null` entry is treated as a missing key (which might or might not be valid depending on the schema).
* complex numbers are represented as size-two arrays containing their real and imaginary part
* JSON does not really distinguish different numeric types. In our convention, the json string `42` is valid as both integer and float, but `42.0` is only a float.

## HDF5

* The top-level type of schema must be `dict` in order to be storable in an hdf5 file.
* Dicts map to groups in HDF5.
* By default, arrays where the elements have numeric type (integers, float, ...) are stored as datasets. 
* TODO: Arrays with other element-types are stored as groups containing keys `"0"`,`"1"`,... Multiple levels deep for multi-dimensional arrays.
* Numeric data (integers, floats, ...) that are not part of an array are stored as "scalar datasets" containing a single element.
