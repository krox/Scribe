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
    schema.validate(value);
    if (tome)
        *tome = value;
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

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                NoneSchema const &)
{
    (void)file;
    (void)path;
    (void)tome;
    throw ValidationError("NoneSchema is never valid");
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                AnySchema const &)
{
    (void)file;
    (void)path;
    (void)tome;
    throw std::runtime_error("AnySchema is not supported for writing");
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                BooleanSchema const &schema)
{
    (void)file;
    (void)path;
    (void)tome;
    (void)schema;
    throw std::runtime_error("not implemented (BooleanSchema)");
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                NumberSchema const &schema)
{
    if (schema.is_integer())
    {
        auto value = tome.as_integer();
        schema.validate(value);
        switch (schema.type)
        {
        case NumType::INT8:
            file.createDataSet<int8_t>(path, value);
            break;
        case NumType::INT16:
            file.createDataSet<int16_t>(path, value);
            break;
        case NumType::INT32:
            file.createDataSet<int32_t>(path, value);
            break;
        case NumType::INT64:
            file.createDataSet<int64_t>(path, value);
            break;
        case NumType::UINT8:
            file.createDataSet<uint8_t>(path, value);
            break;
        case NumType::UINT16:
            file.createDataSet<uint16_t>(path, value);
            break;
        case NumType::UINT32:
            file.createDataSet<uint32_t>(path, value);
            break;
        case NumType::UINT64:
            file.createDataSet<uint64_t>(path, value);
            break;
        default:
            throw std::runtime_error("invalid NumType");
        }
    }
    else if (schema.is_real())
    {
        auto value = tome.as_real();
        schema.validate(value);
        switch (schema.type)
        {
        case NumType::FLOAT32:
            file.createDataSet<float>(path, value);
            break;
        case NumType::FLOAT64:
            file.createDataSet<double>(path, value);
            break;
        default:
            throw std::runtime_error("invalid NumType");
        }
    }
    else
        throw std::runtime_error(
            "invalid NumType (Complex not implemented yet)");
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                StringSchema const &schema)
{
    auto value = tome.as_string();
    schema.validate(value);
    file.createDataSet<std::string>(path, value);
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                ArraySchema const &schema)
{
    auto values = tome.as_array();
    NumberSchema item_schema;
    schema.elements.visit(overloaded{
        [&](NumberSchema const &s) { item_schema = s; },
        [](auto const &) {
            throw std::runtime_error("ArraySchema containing something other "
                                     "than numbers is not implemented yet");
        }});
    if (item_schema.type == NumType::FLOAT64)
    {
        auto shape = values.shape();
        auto dataset = file.createDataSet<double>(
            path, HighFive::DataSpace(values.shape()));
        std::vector<double> data;
        for (auto const &v : values.flat())
            data.push_back(v.as_real());
        dataset.write_raw<double>(data.data());
    }
    else
        throw std::runtime_error("not implemented (ArraySchema containing "
                                 "something other than float64)");
}

void write_impl(HighFive::File &file, std::string const &path, Tome const &tome,
                DictSchema const &schema)
{
    if (!tome.is_dict())
        throw ValidationError("expected a dictionary");
    std::vector<std::string> keys;
    for (auto const &[key, _] : tome.as_dict())
        keys.push_back(key);
    auto item_schemas = schema.validate(keys);
    assert(keys.size() == item_schemas.size());

    if (path != "/")
        file.createGroup(path);
    for (size_t i = 0; i < keys.size(); ++i)
        internal::write_hdf5(file, path + "/" + keys[i],
                             tome.as_dict().at(keys[i]), item_schemas[i]);
}

} // namespace

void scribe::internal::read_hdf5(Tome *tome, HighFive::File &file,
                                 std::string const &path, Schema const &schema)
{
    assert(file.isValid());
    assert(!path.empty() && path.front() == '/');
    schema.visit([&](auto const &s) { read_impl(tome, file, path, s); });
}

void scribe::internal::write_hdf5(HighFive::File &file, std::string const &path,
                                  Tome const &tome, Schema const &schema)
{
    assert(file.isValid());
    assert(!path.empty() && path.front() == '/');
    schema.visit([&](auto const &s) { write_impl(file, path, tome, s); });
}