/**
 *
 * Read/write a memory mapped register
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>

template <typename T>
struct ArrayLength {};

template <typename T, size_t N>
struct ArrayLength<T[N]> {
  static const size_t value = N;
};

template <typename T>
class MemMapRegister {
public:
    T Read() const {
        T tmp;
        for (size_t i = 0; i < len_; ++i) {
            tmp.data[i] = value_.data[i];
        }
        return tmp;
    }

    void Write(const T& value) {
        for (size_t i = 0; i < len_; ++i) {
            value_.data[i] = value.data[i];
        }
    }
private:
    volatile T value_;
    static const size_t len_ = ArrayLength<decltype(T::data)>::value;
};

template <typename T>
struct DefaultBitmap {
    T data[1];

    DefaultBitmap& operator =(const T& value) {
        data[0] = value;
    }
    operator T() const { return data[0]; }
};