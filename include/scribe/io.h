#pragma once

#include "scribe/io_hdf5.h"
#include "scribe/io_json.h"

namespace scribe {

// reading basic types just relays to the reader directly
void read(AtomicType auto &data, Reader auto &reader, std::string_view key)
{
    reader.read(data, key);
}
void read(NumericArrayType auto &data, Reader auto &reader,
          std::string_view key)
{
    reader.read(data, key);
}
template <class T>
void read(std::optional<T> &data, Reader auto &reader, std::string_view key)
{
    reader.read(data, key);
}

// convenience function that enables as user type to only implement key-less
// reading
template <class T> void read(T &data, Reader auto &reader, std::string_view key)
{
    if (key.empty())
    {
        read(data, reader);
    }
    else
    {
        reader.push(key);
        SCRIBE_DEFER(reader.pop());
        read(data, reader);
    }
}

void read_file(auto &data, std::string_view filename)
{
    if (filename.ends_with(".json"))
    {
        auto file = scribe::JsonReader(filename);
        read(data, file);
    }
    else
        throw ReadError("dont recognize file format for " +
                        std::string(filename));
}
} // namespace scribe