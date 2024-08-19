#pragma once

#include <span>
#include <vector>

namespace scribe {

// basic multi-dimensional array
template <class T> class Array
{
    std::vector<T> data_ = {};
    std::vector<size_t> shape_ = {0};
    size_t size_ = 0;

  public:
    Array() = default;
    Array(std::vector<T> data, std::vector<size_t> shape)
        : data_(std::move(data)), shape_(std::move(shape))
    {
        size_ = 1;
        for (auto const &dim : shape_)
            size_ *= dim;
        if (size_ != data_.size())
            throw std::runtime_error("Array: data size does not match shape");
    }

    T *data() { return data_.data(); }
    T const *data() const { return data_.data(); }
    std::vector<size_t> const &shape() const { return shape_; }
    size_t size() const { return size_; }
    size_t rank() const { return shape_.size(); }

    std::span<T> flat() { return data_; }
    std::span<T const> flat() const { return data_; }
    T &flat(size_t i) { return data_.at(i); }
    T const &flat(size_t i) const { return data_.at(i); }

    size_t flat_index(std::span<const size_t> indices) const
    {
        if (indices.size() != shape_.size())
            throw std::runtime_error("Array: wrong number of indices");
        size_t index = 0;
        size_t stride = 1;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            if (indices[i] >= shape_[i])
                throw std::runtime_error("Array: index out of bounds");
            index += indices[i] * stride;
            stride *= shape_[i];
        }
        return index;
    }

    T &operator[](std::span<const size_t> indices)
    {
        return data_[flat_index(indices)];
    }
    T const &operator[](std::span<const size_t> indices) const
    {
        return data_[flat_index(indices)];
    }
};
} // namespace scribe