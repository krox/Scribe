#include "scribe/tome.h"

#include "nlohmann/json.hpp"
#include "scribe/read_json.h"
#include <fstream>

namespace scribe {

Tome Tome::read_json_file(std::string_view filename, Schema const &schema)
{
    Tome tome;
    read_json(&tome,
              nlohmann::json::parse(std::ifstream(std::string(filename)),
                                    nullptr, true, true),
              schema);
    return tome;
}

Tome Tome::read_json_string(std::string_view json, Schema const &schema)
{
    Tome tome;
    read_json(&tome, nlohmann::json::parse(json, nullptr, true, true), schema);
    return tome;
}

Tome Tome::read_file(std::string_view filename, Schema const &schema)
{
    if (filename.ends_with(".json"))
        return read_json_file(filename, schema);
    throw std::runtime_error("unknown file ending");
}
}; // namespace scribe