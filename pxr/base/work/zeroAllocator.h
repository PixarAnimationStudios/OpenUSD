//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_ZERO_ALLOCATOR_H
#define PXR_BASE_WORK_ZERO_ALLOCATOR_H

#include "pxr/pxr.h"

#include <cstring>
#include <type_traits>
#include <memory>

#include <tbb/cache_aligned_allocator.h>

PXR_NAMESPACE_OPEN_SCOPE

/// An allocator that provides zero-initialized memory. 
///
/// \note This meets the standard C++ Allocator requirements and can be passed 
/// as an allocation routine to be used by STL templates. 
///
template <typename T>
struct WorkZeroAllocator : public tbb::cache_aligned_allocator<T> {
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<typename U> struct rebind {
        typedef WorkZeroAllocator<U> other;
    };

    WorkZeroAllocator() = default;

    template <typename U>
    WorkZeroAllocator(const WorkZeroAllocator<U>&) noexcept {};
    
    T* allocate(std::size_t n) {
        T* ptr = tbb::cache_aligned_allocator<T>::allocate(n);
        std::memset(static_cast<void*>(ptr), 0, n * sizeof(value_type));
        return ptr;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif 