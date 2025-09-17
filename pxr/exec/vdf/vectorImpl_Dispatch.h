//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_DISPATCH_H
#define PXR_EXEC_VDF_VECTOR_IMPL_DISPATCH_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include <algorithm>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// This class dispatches copying the vector implementations internal storage
// to a call to memcpy or std::copy, depending on what's appropriate for the
// specific type.
//
template < typename T >
class Vdf_VectorImplDispatch {
public:
    using Memcopyable = std::is_trivially_copyable<T>;

    // Copy from a raw pointer.
    static void Copy(T *dest, const T *source, size_t size) {
        _Copy(dest, source, size, Memcopyable());
    }

    // Copy chunks from a raw pointer, given a bitset indicating what to copy.
    static void Copy(T *dest, const T *source, const VdfMask::Bits &bits) {
        using View = VdfMask::Bits::PlatformsView;
        View platforms = bits.GetPlatformsView();
        for (View::const_iterator it=platforms.begin(), e=platforms.end();
             it != e; ++it) {
            if (it.IsSet()) {
                const size_t index = *it;
                Copy(dest + index, source + index, it.GetPlatformSize());
            }
        }
    }

private:
    // Overloaded _Copy for types that are not memcpy-able.
    template <typename U>
    static void _Copy(
        U *dest, const U *source, size_t size, const std::false_type &) {
        std::copy(source, source + size, dest);
    }

    // Overloaded _Copy for types that ARE memcpy-able.
    template <typename U>
    static void _Copy(
        U *dest, const U *source, size_t size, const std::true_type &) {
        memcpy(dest, source, sizeof(T) * size);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_EXEC_VDF_VECTOR_IMPL_DISPATCH_H */
