//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_REDUCE_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_REDUCE_H

#include <taskflow/taskflow.hpp> 
#include <taskflow/utility/iterator.hpp>
#include <taskflow/algorithm/reduce.hpp>

/// Implements WorkParallelReduce
template <typename Fn, typename Rn, typename V>
V
WorkImpl_ParallelReduceN(
    const V &identity,
    size_t n,
    Fn &&loopCallback,
    Rn &&reductionCallback,
    size_t grainSize)
{
    tf::Taskflow taskflow;
    tf::Executor executor;
    V res = identity;
    taskflow.reduce_by_index(
        tf::IndexRange<size_t>(0, n, 1),
        res,
         [&](tf::IndexRange<size_t> subrange, std::optional<V> runningTotal){
            V residual = runningTotal.has_value() ? *runningTotal : identity;
            return std::forward<Fn>(loopCallback)
                            (subrange.begin(), subrange.end(), residual);
        },
        std::forward<Rn>(reductionCallback));

    executor.run(taskflow).wait();
    return res;
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_REDUCE_H
