//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DEFAULT_INIT_ALLOCATOR
#define PXR_EXEC_VDF_DEFAULT_INIT_ALLOCATOR

#include "pxr/pxr.h"

#include <memory>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Vdf_DefaultInitAllocator
///
/// This allocator intercepts value initialization and turns it into
/// default initialization. It's primary purpose is for use on containers,
/// such as `std::vector`, that are first resized and then immediately 
/// filled with elements. Without the use of this allocator, the resize would
/// value-initialize every element before immediately overwriting the element
/// when it's filled in.
///
template <typename T, typename AllocatorBase = std::allocator<T>>
class Vdf_DefaultInitAllocator : public AllocatorBase
{
public:
    template <typename U>
    struct rebind
    {
        using other = Vdf_DefaultInitAllocator<
            U,
            typename std::allocator_traits<AllocatorBase>::template 
                rebind_alloc<U>>;
    };

    // Make all base class constructors available from this derived class. 
    using AllocatorBase::AllocatorBase;

    /// Value initializing construction. This method instead default
    /// initializes instances of \p U.
    ///
    template <typename U>
    void construct(U* ptr) noexcept(
        std::is_nothrow_default_constructible_v<U>) {
        // Default initialize U
        ::new(static_cast<void*>(ptr)) U;
    }

    /// Construction with custom constructor arguments. This method simply
    /// forwards any arguments to the \p U constructor. 
    ///
    template <typename U, typename...Args>
    void construct(U* ptr, Args&&... args) {
        std::allocator_traits<AllocatorBase>::construct(
            static_cast<AllocatorBase&>(*this),
            ptr,
            std::forward<Args>(args)...);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
