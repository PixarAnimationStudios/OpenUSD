//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_REDUCE_IMPL_H
#define PXR_BASE_WORK_TBB_REDUCE_IMPL_H

#include "pxr/pxr.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_group.h>

PXR_NAMESPACE_OPEN_SCOPE

/// TBB Parallel Reduce Implementation
/// 
/// Implements WorkParallelReduceN
///
template <typename Fn, typename Rn, typename V>
V
WorkImpl_ParallelReduceN(
    const V &identity,
    size_t n,
    Fn &&loopCallback,
    Rn &&reductionCallback,
    size_t grainSize)
{

    class Work_Body_TBB
    {
    public:
        Work_Body_TBB(Fn &fn) : _fn(fn) { }

        V operator()(
            const tbb::blocked_range<size_t> &r,
            const V &value) const {
            // Note that we std::forward _fn using Fn in order get the
            // right operator().
            // We maintain the right type in this way:
            //  If Fn is T&, then reference collapsing gives us T& for _fn
            //  If Fn is T, then std::forward correctly gives us T&& for _fn
            return std::forward<Fn>(_fn)(r.begin(), r.end(), value);
        }
    private:
        Fn &_fn;
    };

    // In most cases we do not want to inherit cancellation state from the
    // parent context, so we create an isolated task group context.
    tbb::task_group_context ctx(tbb::task_group_context::isolated);
    return tbb::parallel_reduce(tbb::blocked_range<size_t>(0,n,grainSize),
        identity,
        Work_Body_TBB(loopCallback),
        std::forward<Rn>(reductionCallback),
        tbb::auto_partitioner(),
        ctx);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_TBB_REDUCE_IMPL_H
