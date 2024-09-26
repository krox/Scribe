#include "scribe/tome.h"

#include "scribe/io_hdf5.h"
#include "scribe/io_json.h"
#include <fstream>

void scribe::read_file(Tome &tome, std::string_view filename,
                       Schema const &schema)
{
    if (filename.ends_with(".json"))
    {
        auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                       nullptr, true, true);
        internal::read_json(&tome, j, schema);
    }
    else if (filename.ends_with(".h5") || filename.ends_with(".hdf5"))
    {
        auto file =
            HighFive::File(std::string(filename), HighFive::File::ReadOnly);
        internal::read_hdf5(&tome, file, "/", schema);
    }
    else
        throw std::runtime_error("unknown file ending when reading a file");
}

void scribe::write_file(std::string_view filename, Tome const &tome,
                        Schema const &schema)
{
    if (filename.ends_with(".json"))
    {
        nlohmann::json j;
        internal::write_json(j, tome, schema);
        std::ofstream(std::string(filename)) << j.dump(4) << '\n';
    }
    else if (filename.ends_with(".h5") || filename.ends_with(".hdf5"))
    {
        auto file =
            HighFive::File(std::string(filename), HighFive::File::ReadWrite |
                                                      HighFive::File::Create |
                                                      HighFive::File::Truncate);
        internal::write_hdf5(file, "/", tome, schema);
    }
    else
        throw std::runtime_error("unknown file ending when writing a file");
}

void scribe::read_json_string(Tome &tome, std::string_view json,
                              Schema const &schema)
{
    auto j = nlohmann::json::parse(json, nullptr, true, true);
    internal::read_json(&tome, j, schema);
}

void scribe::write_json_string(std::string &s, Tome const &tome,
                               Schema const &schema)
{
    nlohmann::json j;
    internal::write_json(j, tome, schema);
    s = j.dump(4);
}

void scribe::validate_file(std::string_view filename, Schema const &s)
{
    if (filename.ends_with(".json"))
    {
        auto j = nlohmann::json::parse(std::ifstream(std::string(filename)),
                                       nullptr, true, true);
        internal::read_json(nullptr, j, s);
    }
    else
        throw std::runtime_error("unknown file ending when validating a file");
}
