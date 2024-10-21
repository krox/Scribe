# minimum-viable-product checklist

This is a somewhat-detailed list of all the features to be implemented for the first actually useful version of scribe, which might be called "1.0" then.

## Tome type

* [ ] finalize and document overall design of `Tome` (e.g. default=dict? no nulls? implicit conversions?)
* [x] use `xtensor` as array backend.
* [x] use `std::map<std::string, ...>` as backend for dicts
* [x] compact "array-of-numbers" implementation for integer/float/complex.

## Schema validation
Validation has to happen both with the exicit `scribe validate` command, as well as the `scribe::read(...)` and `scribe::write(...)` functions that take a schema. Un-validated read/write only does basic consistency checks.
* [x] string validation: min/max length
* [x] integer validation: implicit range of type
* [x] array validation: check shape and recurse to elements
* [x] dict validation: check keys and recurse to elements
* [ ] validation errors should indicate a (human-readable) location of the failure
* [ ] ~~strict mode: more precise type matching, checks chunking, checks presence of checksum~~ no, validation should be strictly black and white. If chunksize is part of the schema, it has to match.
* [x] schema documentation. See `docs/schema.md`
* [ ] write a json-schema for the format of a schema itself.
  
## Reading/Writing
### JSON
* [ ] integers: precise domains according to the type (e.g. `[-128,127]` for `int8`). Needs careful attention to nlohmann's handling of numbers close to the limits.
* [x] floats
* [x] complex
* [x] strings
* [x] arrays. Assume everything 1D when no schema is given.
* [ ] dicts. Make sure to reject duplicate keys.
* [ ] 'any' schema reading


### HDF5
* [ ] float: precision has to match exactly
* [ ] integer: exact width/signdness match
* [ ] complex, string: document precise mapping (because these are not part of the HDF5 spec). We follow what HighFive does by default. Check how Grid saves complex numbers. MIght need flag in schema/traits/hdf5
* [x] arrays of numbers via datasets
* [ ] arrays of non-numbers via groups (potentially nested)
* [ ] activate chunking (single-chunk is okay as a default)
* [ ] activate fletcher32
* [x] reading with 'any' schema

## Code generation

Generate C++ code using the `scribe codegen` command.

* [x] basic header generation
* [ ] better automatic names for nested structs
* [ ] implementation for `read` and `write` functions, calling out to `libscribe` functions
  
## Testing infrastructure

* [ ] set up automatic unittests using github-runner for 
  * [ ] linux gcc
  * [ ] linux clang
  * [ ] macos
  * [ ] windows (optional)
* [x] unit-tests for the tome-type on its own, using it as a generic container
* [x] unit-tests for json-reading/writing
* [ ] unit-tests for hdf5-reading/writing
* [ ] integration test: example project using `scribe codegen` from `CMake`
* [ ] integration test: calling `scribe validate` and `scribe convert`
* [ ] systematic unit-tests for every explicit constraint in a schema. Should be testable on the level of `Tome`, without touching json or hdf5.


## Some still open design decisions
* [ ] What should `Tome::operator[]` return? The obvious `Tome &` does not quite work with compact numerical arrays. Some options:
  * invent a new `TomeRef` (`ConstTomeRef`) class that can refer to a single number or a Tome
  * disallow mutable `operator[]` for compact arrays and invent a new `.index<T>(...) -> T&` method for that (throwing on type mismatch)
* [ ] Should we interpred non-existing arrays as `[]`?
* [ ] Should we interpred non-existing dicts as `{}`?
* [ ] Should we interpred non-existing strings as `""`?
* [ ] What does a (multi-dimensional) array map to when generating C++ code? Some options:
  * ~~`std::vector<std::vector< ... >>`~~
  * ~~`Tome::array<...>`~~
  * ~~switch between the two depending on rank~~
  * ~~disallow multi-dimensional arrays unless a mapping was specified in the schema~~
  * default to `xtensor`, regardless of rank
