#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"

namespace scribe {

// returns true if j follows the schema s
bool verify_json_file(std::string_view filename, Schema const &s,
                      bool verbose = false);

} // namespace scribe