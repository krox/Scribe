#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/tome.h"

namespace scribe {

// returns true if j follows the schema s
void validate_json_file(std::string_view filename, Schema const &s);

} // namespace scribe