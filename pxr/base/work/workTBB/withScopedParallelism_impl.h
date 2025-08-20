//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_WITH_SCOPED_PARALLELISM_IMPL_H
#define PXR_BASE_WORK_TBB_WITH_SCOPED_PARALLELISM_IMPL_H

#include <tbb/task_arena.h>

#include "pxr/pxr.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// TBB Scoped Parallelism Implementation
///
/// Implements WorkWithScopedParallelism
///
template <class Fn>
auto
WorkImpl_WithScopedParallelism(Fn &&fn)
{
    return tbb::this_task_arena::isolate(std::forward<Fn>(fn));
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_TBB_WITH_SCOPED_PARALLELISM_IMPL_H

