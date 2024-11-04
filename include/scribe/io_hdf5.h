#pragma once

#include "scribe/schema.h"
#include "scribe/tome.h"

#include "highfive/highfive.hpp"

namespace scribe {
namespace internal {
// validates and reads a JSON object according to the given schema
//   * throws ValidationError if the JSON object does not follow the schema
//   * set tome=nullptr to only validate
void read_hdf5(Tome *, HighFive::File &, std::string const &path,
               Schema const &);

void write_hdf5(HighFive::File &, std::string const &path, Tome const &,
                Schema const &);
} // namespace internal

class Hdf5Reader
{
    HighFive::File file_;
    std::vector<HighFive::Group> stack_;
    std::vector<std::string> keys_;

    HighFive::Group const &current() const { return stack_.back(); }

  public:
    Hdf5Reader(Hdf5Reader const &) = delete;
    Hdf5Reader &operator=(Hdf5Reader const &) = delete;

    explicit Hdf5Reader(std::string_view filename)
    try : file_(std::string(filename), HighFive::File::ReadOnly)
    {
        stack_.push_back(file_.getGroup("/"));
    }
    catch (HighFive::FileException const &e)
    {
        throw ReadError(e.what());
    }

    // human-readable location in the JSON file
    std::string current_path() const
    {
        return fmt::format("/{}", fmt::join(keys_, "/"));
    }

    void push(std::string_view key_)
    {
        // HighFive does not like string_view's :(
        auto key = std::string(key_);

        assert(!key.empty());
        if (!current().exist(key))
            throw ReadError("missing key '" + std::string(key) + "' at " +
                            current_path());
        stack_.push_back(current().getGroup(key));
        keys_.push_back(key);
    }

    void pop() noexcept
    {
        assert(stack_.size() > 1);
        keys_.pop_back();
        stack_.pop_back();
    }

    void read(AtomicType auto &value, std::string_view key_)
    {
        auto key = std::string(key_);
        auto dset = current().getDataSet(key);
        dset.read(value);
    }

    template <class T> void read(std::optional<T> &value, std::string_view key_)
    {
        auto key = std::string(key_);
        assert(!key.empty());
        if (!current().exist(key))
        {
            value.reset();
            return;
        }

        if (!value)
            value.emplace();
        read(*value, key);
    }

    void read(NumericArrayType auto &value, std::string_view key_)
    {
        auto key = std::string(key_);
        auto dset = current().getDataSet(key);
        value.resize(dset.getSpace().getDimensions());
        dset.read(value.data());
    }
};

static_assert(Reader<Hdf5Reader>);

} // namespace scribe