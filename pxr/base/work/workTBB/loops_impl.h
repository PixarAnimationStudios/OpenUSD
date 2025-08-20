//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_LOOPS_IMPL_H
#define PXR_BASE_WORK_TBB_LOOPS_IMPL_H

#include "pxr/pxr.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/task_group.h>

PXR_NAMESPACE_OPEN_SCOPE

/// TBB Parallel For Implementation
///
/// Implements ParallelForN
///
template <typename Fn>
void
WorkImpl_ParallelForN(size_t n, Fn &&callback, size_t grainSize)
{
    class Work_ParallelForN_TBB 
    {
    public:
        Work_ParallelForN_TBB(Fn &fn) : _fn(fn) { }

        void operator()(const tbb::blocked_range<size_t> &r) const {
            // Note that we std::forward _fn using Fn in order get the
            // right operator().
            // We maintain the right type in this way:
            //  If Fn is T&, then reference collapsing gives us T& for _fn 
            //  If Fn is T, then std::forward correctly gives us T&& for _fn
            std::forward<Fn>(_fn)(r.begin(), r.end());
        }

    private:
        Fn &_fn;
    };

    // In most cases we do not want to inherit cancellation state from the
    // parent context, so we create an isolated task group context.
    tbb::task_group_context ctx(tbb::task_group_context::isolated);
    tbb::parallel_for(tbb::blocked_range<size_t>(0,n,grainSize),
        Work_ParallelForN_TBB(callback),
        ctx);
}

/// Implements WorkParallelForEach
///
template <typename InputIterator, typename Fn>
inline void
WorkImpl_ParallelForEach(
    InputIterator first, InputIterator last, Fn &&fn)
{
    tbb::task_group_context ctx(tbb::task_group_context::isolated);
    tbb::parallel_for_each(first, last, std::forward<Fn>(fn), ctx);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_TBB_LOOPS_IMPL_H
