#include "scribe/validate_json.h"

#include "scribe/read_json.h"
#include <fstream>

namespace scribe {
void validate_json_file(std::string_view filename, Schema const &s)
{
    auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                   nullptr, true, true);
    read_json(nullptr, j, s);
}
} // namespace scribe