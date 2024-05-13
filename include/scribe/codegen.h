#pragma once

#include "scribe/schema.h"

namespace scribe {
std::string generate_cpp(Schema const &schema);
}