#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "scribe/codegen.h"
#include "scribe/io_json.h"
#include "scribe/schema.h"
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
    std::string out_filename;
    bool verbose = false;

    auto validate_command = app.add_subcommand(
        "validate", "validate a data file (json/hdf5) against a schema");
    validate_command->add_option("--schema", schema_filename, "schema file")
        ->required();
    validate_command->add_option("data", data_filename, "data file")
        ->required();
    validate_command->add_flag("--verbose,-v", verbose, "verbose output");

    auto codegen_command =
        app.add_subcommand("codegen", "generate C++ code from a schema");
    codegen_command->add_option("--schema", schema_filename, "schema file")
        ->required();

    auto convert_command = app.add_subcommand(
        "convert", "convert a data file from one format to another");
    convert_command->add_option("--schema", schema_filename, "schema file");
    convert_command->add_option("in", data_filename, "input file")->required();
    convert_command->add_option("out", out_filename, "output file")->required();

    auto guess_schema_command = app.add_subcommand(
        "guess-schema", "guess a schema from a data file (hdf5 only)");
    guess_schema_command->add_option("data", data_filename, "data file (input)")
        ->required();
    guess_schema_command->add_option("schema", schema_filename,
                                     "schema file (output. default to stdout)");

    CLI11_PARSE(app, argc, argv);

    if (validate_command->parsed())
    {
        auto schema = scribe::Schema::from_file(schema_filename);
        try
        {
            validate_file(data_filename, schema);
            fmt::print("validation OK\n");
            return 0;
        }
        catch (scribe::ValidationError const &e)
        {
            fmt::print("validation FAILED: {}\n", e.what());
            return 1;
        }
    }
    else if (codegen_command->parsed())
    {
        auto schema = scribe::Schema::from_file(schema_filename);
        fmt::print("{}\n", generate_cpp(schema));
        return 0;
    }
    else if (convert_command->parsed())
    {
        auto schema = schema_filename.empty()
                          ? Schema::any()
                          : scribe::Schema::from_file(schema_filename);
        Tome tome;
        read_file(tome, data_filename, schema);
        write_file(out_filename, tome, schema);
    }
    else if (guess_schema_command->parsed())
    {
        Tome tome;
        read_file(tome, data_filename, Schema::any());
        auto schema = guess_schema(tome);
        if (schema_filename.empty())
        {
            fmt::print("{}\n", schema.to_json().dump(4));
        }
        else
        {
            auto file = std::ofstream(schema_filename);
            file << schema.to_json().dump(4) << '\n';
        }
    }
    else
    {
        assert(false);
    }
}
