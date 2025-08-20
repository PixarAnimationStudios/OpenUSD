//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SORT_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SORT_H

#include <taskflow/taskflow.hpp> 
#include <taskflow/algorithm/sort.hpp>

/// Implements WorkParallelSort
template <typename C>
void 
WorkImpl_ParallelSort(C* container)
{
    tf::Taskflow taskflow;
    tf::Executor executor;
    tf::Task sort = taskflow.sort(container->begin(), container->end());
    executor.run(taskflow).wait();
}

/// Implements WorkParallelSort with a custom comparison functor.
template <typename C, typename Compare>
void 
WorkImpl_ParallelSort(C* container, const Compare& comp)
{
    tf::Taskflow taskflow;
    tf::Executor executor;
    tf::Task sort = taskflow.sort(container->begin(), container->end(), comp);
    executor.run(taskflow).wait();
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SORT_H