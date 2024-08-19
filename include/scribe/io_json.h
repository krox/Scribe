#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/tome.h"

namespace scribe {

// validates and reads a JSON object according to the given schema
//   * throws ValidationError if the JSON object does not follow the schema
//   * set tome=nullptr to only validate
void read_json(Tome *, nlohmann::json const &, Schema const &);

// writes a JSON object according to the given schema
void write_json(nlohmann::json &, Tome const &, Schema const &);

// throws ValidationError if the JSON file does not follow the schema
void validate_json_file(std::string_view filename, Schema const &s);

// TODO: reading/writing directcly from/to a file, without going through an
// nlohmann::json object

} // namespace scribe