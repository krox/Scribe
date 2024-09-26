#pragma once

#include "scribe/schema.h"
#include "scribe/tome.h"

#include "highfive/highfive.hpp"

namespace scribe {
namespace internal {
// validates and reads a JSON object according to the given schema
//   * throws ValidationError if the JSON object does not follow the schema
//   * set tome=nullptr to only validate
void read_hdf5(Tome *, HighFive::File &, std::string const &path,
               Schema const &);
} // namespace internal
} // namespace scribe