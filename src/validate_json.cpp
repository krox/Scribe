#include "scribe/validate_json.h"

#include "scribe/read_json.h"
#include <fstream>

namespace scribe {
bool validate_json_file(std::string_view filename, Schema const &s,
                        bool verbose)
{
    (void)verbose;
    auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                   nullptr, true, true);
    return read_json(nullptr, j, s);
}
} // namespace scribe