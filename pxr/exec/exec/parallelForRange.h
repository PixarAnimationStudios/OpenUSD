//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PARALLEL_FOR_RANGE_H
#define PXR_EXEC_EXEC_PARALLEL_FOR_RANGE_H

#include "pxr/pxr.h"

#include "pxr/base/work/withScopedParallelism.h"

#include <tbb/blocked_range.h>

#include <functional>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Concurrently invokes \p callable on all entries of \p container, as long as
/// \p Container supports the TBB range concept (e.g.,
/// tbb::concurrent_unordered_map). 
/// 
template <class Container, class Callable>
void Exec_ParallelForRange(Container &container, Callable &&callable)
{
    // Will be range_type or const_range_type based on cv-qualifiers of
    // container.
    using RangeType = decltype(container.range());

    // The parallel task responsible for recursively sub-dividing the range
    // and invoking the callable on the sub-ranges.
    class _RangeTask
    {
    public:
        _RangeTask(
            WorkDispatcher &dispatcher,
            RangeType &&range,
            const Callable &callable)
        : _dispatcher(dispatcher)
        , _range(std::move(range))
        , _callable(callable) {}

        void operator()() const {
            // Subdivide the given range until it is no longer divisible, and
            // recursively spawn _RangeTasks for the right side of the split.
            RangeType &leftRange = _range;
            while (leftRange.is_divisible()) {
                RangeType rightRange(leftRange, tbb::split());
                _dispatcher.Run(_RangeTask(
                    _dispatcher, std::move(rightRange), _callable));
            }

            // If there are any more entries remaining in the left-most side
            // of the given range, invoke the callable on the left-most range.
            if (!leftRange.empty()) {
                std::invoke(_callable, leftRange);
            }
        }

    private:
        WorkDispatcher &_dispatcher;
        mutable RangeType _range;
        const Callable &_callable;
    };

    WorkWithScopedDispatcher(
        [&container, &callable](WorkDispatcher &dispatcher){
        RangeType range = container.range();
        dispatcher.Run(_RangeTask(
            dispatcher, std::move(range), std::forward<Callable>(callable)));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
