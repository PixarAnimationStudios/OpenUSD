//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ESTIMATE_SIZE_H
#define PXR_EXEC_VDF_ESTIMATE_SIZE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Estimate the memory footprint of instance \p t of type T.
///
/// This function estimates the memory footprint of an instance of type T.
/// Internally this is used to total up cache sizes, and give the system an
/// opportunity to limit the memory used for execution caches.
///
/// By default, this function invokes the sizeof() operator for type T. The
/// returned value is not a good estimate of the true memory footprint, if 
/// instances of T allocate heap memory.
///
/// Plugin writers can *overload* (not specialize) VdfEstimateSize for their own
/// types in order to provide a better estimate. Note that in order for ADL to
/// be guaranteed to find the new overload, the pattern is to declare it as an
/// inline friend within the namespace of the class itself. Further, not
/// polluting the unqualified namespace with these overloads also results in
/// more succinct compiler errors if overload resolution were to fail.
/// 
/// Please follow the pattern established in this example:
///
/// ```{.cpp}
/// struct MyType {
///     uint8_t *data;
///     size_t numData;
///     
///     friend size_t
///     VdfEstimateSize(const MyType &t)
///     { 
///         return sizeof(MyTpe) + (sizeof(uint8_t) * t.numData);
///     }
/// };
/// ```
///
template < typename T >
inline size_t
VdfEstimateSize(const T &)
{
    return sizeof(T);
}

/// Overload for TfSmallVector<T, N>
///
template < typename T, uint32_t N >
inline size_t
VdfEstimateSize(const TfSmallVector<T, N> &v)
{
    // It would be more accurate the iterate over v and estimate the size of
    // each element of v, but we are optimizing for performance rather than
    // accuracy for now.
    const size_t numExternal = (v.capacity() > N) ? v.capacity() : 0;
    const size_t elementSize = v.empty()
        ? sizeof(T) : VdfEstimateSize(v.front());
    return sizeof(TfSmallVector<T, N>) + numExternal * elementSize;
}

/// Overload for std::vector<T>
///
template < typename T, typename A >
inline size_t
VdfEstimateSize(const std::vector<T, A> &v)
{
    // It would be more accurate the iterate over v and estimate the size
    // of each element of v, but we are optimizing for performance rather
    // than accuracy for now.
    const size_t elementSize = v.empty()
        ? sizeof(T) : VdfEstimateSize(v.front());
    return sizeof(std::vector<T, A>) + v.capacity() * elementSize;
}

/// Overload for std::shared_ptr<T>
///
template < typename T >
inline size_t
VdfEstimateSize(const std::shared_ptr<T> &p)
{
    return sizeof(std::shared_ptr<T>) + (p ? VdfEstimateSize(*p) : 0);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_EXEC_VDF_ESTIMATE_SIZE_H */
