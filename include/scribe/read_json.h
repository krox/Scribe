#pragma once

#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/tome.h"

namespace scribe {

// returns true if j follows the schema s
bool read_json(Tome *, nlohmann::json const &, Schema const &);

} // namespace scribe