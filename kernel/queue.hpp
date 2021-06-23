#pragma once

#include <cstddef>
#include <array>

#include "error.hpp"

template <typename T>
class ArrayQueue {
public:
    template <size_t N>
    ArrayQueue(std::array<T, N>& buf);
    ArrayQueue(T* buf, size_t size);
    Error Push(const T& value);
    Error Pop();
    size_t Count() const;
    size_t Capacity() const;
    const T& Front() const;

private:
    T* data_;
    size_t read_pos_, write_pos_;
    const size_t capacity_;
};

template <typename T>
template <size_t N>
ArrayQueue<T>::ArrayQueue(std::array<T, N>& buf):
    ArrayQueue(buf.data(), N)
{}

template <typename T>
ArrayQueue<T>::ArrayQueue(T* buf, size_t size):
    data_{buf}, read_pos_{0}, write_pos_{0}, capacity_{size}
{}

template <typename T>
Error ArrayQueue<T>::Push(const T& value) {
    if(Count() >= capacity_) return MAKE_ERROR(Error::kFull);
    data_[write_pos_] = value;
    (++write_pos_) %= capacity_;

    return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
Error ArrayQueue<T>::Pop() {
    if(Count() <= 0) return MAKE_ERROR(Error::kEmpty);
    (++read_pos_) %= capacity_;

    return MAKE_ERROR(Error::kSuccess);
}

template <typename T>
size_t ArrayQueue<T>::Count() const {
    return ((write_pos_ - read_pos_) + capacity_) % capacity_;
}

template <typename T>
size_t ArrayQueue<T>::Capacity() const {
    return capacity_;
}

template <typename T>
const T& ArrayQueue<T>::Front() const {
    return data_[read_pos_];
}
