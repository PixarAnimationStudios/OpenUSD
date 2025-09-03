//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_SORT_H
#define PXR_BASE_WORK_SORT_H

/// \file work/sort.h

#include "pxr/pxr.h"
#include "pxr/base/work/impl.h"
#include "pxr/base/work/threadLimits.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

/// Sorts in-place a container that provides begin() and end() methods
///
template <typename C>
void 
WorkParallelSort(C* container)
{
    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        WorkImpl_ParallelSort(container);
    }else{
        std::sort(container->begin(), container->end());
    }
}


/// Sorts in-place a container that provides begin() and end() methods,
/// using a custom comparison functor.
///
template <typename C, typename Compare>
void 
WorkParallelSort(C* container, const Compare& comp)
{
    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        WorkImpl_ParallelSort(container, comp);
    }else{
        std::sort(container->begin(), container->end(), comp);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
