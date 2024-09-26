#include "scribe/io_hdf5.h"

#include "highfive/highfive.hpp"

namespace {
using namespace scribe;

void read_impl(Tome *, HighFive::File &, std::string const &,
               NoneSchema const &)
{
    throw ValidationError("NoneSchema is never valid");
}
void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               AnySchema const &)
{
    (void)tome;
    (void)file;
    (void)path;
    if (!file.exist(path))
        throw ReadError(fmt::format("object '{}' does not exist", path));
    if (tome)
        throw ReadError("AnySchema is not supported for reading");
}

void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               BooleanSchema const &schema)
{
    (void)tome;
    (void)file;
    (void)path;
    (void)schema;
    throw std::runtime_error("not implemented (BooleanSchema)");
}

void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               NumberSchema const &schema)
{
    // NOTE: a raw number (not in a homogeneous array) is stored as a scalar
    // dataset in HDF5
    if (!file.exist(path))
        throw ReadError(fmt::format("object '{}' does not exist", path));
    auto dataset = file.getDataSet(path);
    if (dataset.getElementCount() != 1)
        throw ReadError(fmt::format("expected scalar dataset at '{}'", path));
    if (schema.is_integer())
    {
        auto value = dataset.read<int64_t>();
        schema.validate(value);
        if (tome)
            *tome = value;
    }
    else if (schema.is_real())
    {
        auto value = dataset.read<double>();
        schema.validate(value);
        if (tome)
            *tome = value;
    }
    else
        throw std::runtime_error(
            "invalid NumType (Complex not implemented yet)");
}

void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               StringSchema const &schema)
{
    if (!file.exist(path))
        throw ReadError(fmt::format("object '{}' does not exist", path));
    auto dataset = file.getDataSet(path);
    auto value = dataset.read<std::string>();
    if (tome)
        *tome = value;
    (void)schema;
}

void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               ArraySchema const &schema)
{
    if (!file.exist(path))
        throw ReadError(fmt::format("object '{}' does not exist", path));
    NumberSchema item_schema;
    schema.elements.visit(overloaded{
        [&](NumberSchema const &s) { item_schema = s; },
        [](auto const &) {
            throw std::runtime_error("ArraySchema containing something other "
                                     "than numbers is not implemented yet");
        }});

    auto dataset = file.getDataSet(path);
    auto shape = dataset.getDimensions();
    size_t size = dataset.getElementCount();
    schema.validate_shape(shape);
    if (item_schema.is_real())
    {
        auto values = std::vector<double>(size);
        dataset.read_raw(values.data());
        if (tome)
            *tome = Tome::array(std::move(values), shape);
    }
}

void read_impl(Tome *tome, HighFive::File &file, std::string const &path,
               DictSchema const &schema)
{
    assert(file.isValid());
    if (!file.exist(path))
        throw ReadError(fmt::format("object '{}' does not exist", path));
    auto group = file.getGroup(path);

    // get and validate list of items inside this group
    std::vector<std::string> item_keys = group.listObjectNames();
    std::vector<Schema> item_schemas = schema.validate(item_keys);
    assert(item_keys.size() == item_schemas.size());

    // read and validate each item
    if (tome)
        *tome = Tome::dict();
    for (size_t i = 0; i < item_keys.size(); ++i)
        internal::read_hdf5(tome ? &tome->as_dict()[item_keys[i]] : nullptr,
                            file, path + "/" + item_keys[i], item_schemas[i]);
}

} // namespace

void scribe::internal::read_hdf5(Tome *tome, HighFive::File &file,
                                 std::string const &path, Schema const &schema)
{
    assert(file.isValid());
    assert(!path.empty() && path.front() == '/');
    schema.visit([&](auto const &s) { read_impl(tome, file, path, s); });
}
