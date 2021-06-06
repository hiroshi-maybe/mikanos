#pragma once

#include <cstddef>

namespace usb {

static const size_t kMemoryPoolSize = 4096 * 32;

/**
 * Allocate the size of memory and returns a pointer of them
 */
void* AllocMem(size_t size, unsigned int alignment, unsigned int boundary);

template <class T>
T* AllocArray(size_t num_obj, unsigned int alignment, unsigned int boundary) {
    return reinterpret_cast<T*>(AllocMem(sizeof(T) * num_obj, alignment, boundary));
}

void FreeMem(void* p);

}