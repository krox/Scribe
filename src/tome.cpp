#include "scribe/tome.h"

#include "nlohmann/json.hpp"
#include "scribe/io_json.h"
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

void Tome::write_file(std::string_view filename, Schema const &schema) const
{
    if (filename.ends_with(".json"))
    {
        write_json_file(filename, schema);
        return;
    }
    throw std::runtime_error("unknown file ending");
}

void Tome::write_json_file(std::string_view filename,
                           Schema const &schema) const
{
    nlohmann::json j;
    write_json(j, *this, schema);
    std::ofstream(std::string(filename)) << j.dump(4) << '\n';
}
}; // namespace scribe