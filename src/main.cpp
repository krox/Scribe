#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "scribe/schema.h"
#include "scribe/verify_json.h"
#include <cassert>
#include <fstream>

using namespace scribe;
using json = nlohmann::json;

int main(int argc, char **argv)
{
    CLI::App app{"Scribe data schema."};
    app.require_subcommand(1);

    std::string schema_filename;
    std::string data_filename;
    bool verbose = false;

    auto verify_command = app.add_subcommand(
        "verify", "verify a data file (json/hdf5) against a schema");
    verify_command->add_option("schema", schema_filename, "schema file")
        ->required();
    verify_command->add_option("data", data_filename, "data file")->required();
    verify_command->add_flag("--verbose,-v", verbose, "verbose output");

    CLI11_PARSE(app, argc, argv);

    if (verify_command->parsed())
    {
        auto schema = scribe::Schema::from_file(schema_filename);
        auto result = verify_json_file(data_filename, schema, verbose);
        if (result)
        {
            fmt::print("verification OK\n");
            return 0;
        }
        else
        {
            fmt::print("verification FAILED\n");
            return 1;
        }
    }
    else
    {
        assert(false);
    }
}
