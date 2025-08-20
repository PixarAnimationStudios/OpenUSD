//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_LOOPS_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_LOOPS_H

#include <taskflow/taskflow.hpp> 
#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/utility/iterator.hpp>

/// Implements WorkParallelForN
template <typename Fn>
void
WorkImpl_ParallelForN(size_t n, Fn &&callback, size_t grainSize)
{
    tf::Taskflow taskflow;
    tf::Executor executor;
    tf::IndexRange<size_t> range(0, n, 1);
    taskflow.for_each_by_index(range, [&](tf::IndexRange<size_t> subrange){
        std::forward<Fn>(callback)(subrange.begin(),subrange.end());
    });
    executor.run(taskflow).wait();
}

/// Implements WorkParallelForEach
template <typename InputIterator, typename Fn>
void
WorkImpl_ParallelForEach(
    InputIterator first, InputIterator last, Fn &&fn)
{
    tf::Taskflow taskflow;
    // TODO: Currently, spawning more than 1 thread to apply fn to when the 
    // InputIterator contains UsdPrims causes those prims to becom invalid in the 
    // body of the function. Better if we could spawn more. 
    tf::Executor executor(1);
    taskflow.for_each(first, last, std::forward<Fn>(fn));
    executor.run(taskflow).wait();
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_LOOPS_H
