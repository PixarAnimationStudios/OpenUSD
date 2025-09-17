//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_LOOPS_H
#define PXR_BASE_WORK_LOOPS_H

/// \file work/loops.h
#include "pxr/pxr.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/impl.h"
#include "pxr/base/work/threadLimits.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// WorkSerialForN(size_t n, CallbackType callback)
///
/// A serial version of WorkParallelForN as a drop in replacement to
/// selectively turn off multithreading for a single parallel loop for easier
/// debugging.
///
/// Callback must be of the form:
///
///     void LoopCallback(size_t begin, size_t end);
///
template<typename Fn>
void
WorkSerialForN(size_t n, Fn &&fn)
{
    std::forward<Fn>(fn)(0, n);
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelForN(size_t n, CallbackType callback, size_t grainSize = 1)
///
/// Runs \p callback in parallel over the range 0 to n.
///
/// Callback must be of the form:
///
///     void LoopCallback(size_t begin, size_t end);
///
/// grainSize specifies a minimum amount of work to be done per-thread. There
/// is overhead to launching a thread (or task) and a typical guideline is that
/// you want to have at least 10,000 instructions to count for the overhead of
/// launching a thread.
///
template <typename Fn>
void
WorkParallelForN(size_t n, Fn &&callback, size_t grainSize)
{
    if (n == 0)
        return;

    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        WorkImpl_ParallelForN(n, std::forward<Fn>(callback), grainSize);
    } else {
        // If concurrency is limited to 1, execute serially.
        WorkSerialForN(n, std::forward<Fn>(callback));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelForN(size_t n, CallbackType callback, size_t grainSize = 1)
///
/// Runs \p callback in parallel over the range 0 to n.
///
/// Callback must be of the form:
///
///     void LoopCallback(size_t begin, size_t end);
///
///
template <typename Fn>
void
WorkParallelForN(size_t n, Fn &&callback)
{
    WorkParallelForN(n, std::forward<Fn>(callback), 1);
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelForTBBRange(const RangeType &r, Fn &&callback)
///
/// Runs \p callback in parallel over a RangeType that adheres to TBB's
/// splittable range requirements: 
/// https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/named_requirements/algorithms/range
///
/// Callback must be of the form:
///
///     void LoopCallback(RangeType range);
///
///
template <typename RangeType, typename Fn>
void
WorkParallelForTBBRange(const RangeType &range, Fn &&callback)
{
    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        // Use the work backend's ParallelForTBBRange if one exists
        // otherwise use the default implementation below that builds off of the 
        // dispatcher.
#if defined WORK_IMPL_HAS_PARALLEL_FOR_TBB_RANGE
        WorkImpl_ParallelForTBBRange(range, std::forward<Fn>(callback));
#else
        // The parallel task responsible for recursively sub-dividing the range
        // and invoking the callback on the sub-ranges.
        class _RangeTask
        {
        public:
            _RangeTask(
                WorkDispatcher &dispatcher,
                RangeType &&range,
                const Fn &callback)
            : _dispatcher(dispatcher)
            , _range(std::move(range))
            , _callback(callback) {}

            void operator()() const {
                // Subdivide the given range until it is no longer divisible, and
                // recursively spawn _RangeTasks for the right side of the split.
                RangeType &leftRange = _range;
                while (leftRange.is_divisible()) {
                    RangeType rightRange(leftRange, tbb::split());
                    _dispatcher.Run(_RangeTask(
                        _dispatcher, std::move(rightRange), _callback));
                }

                // If there are any more entries remaining in the left-most side
                // of the given range, invoke the callback on the left-most range.
                if (!leftRange.empty()) {
                    std::invoke(_callback, leftRange);
                }
            }

        private:
            WorkDispatcher &_dispatcher;
            mutable RangeType _range;
            const Fn &_callback;
        };

        WorkDispatcher dispatcher;
        RangeType range = range;
        dispatcher.Run(_RangeTask(
            dispatcher, range, std::forward<Fn>(callback)));
#endif
    } else {
        // If concurrency is limited to 1, execute serially.
        std::forward<Fn>(callback)(range);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelForEach(Iterator first, Iterator last, CallbackType callback)
///
/// Callback must be of the form:
///
///     void LoopCallback(T elem);
///
/// where the type T is deduced from the type of the InputIterator template
/// argument.
///
/// 
template <typename InputIterator, typename Fn>
inline void
WorkParallelForEach(
    InputIterator first, InputIterator last, Fn &&fn)
{
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        WorkImpl_ParallelForEach(first, last, std::forward<Fn>(fn));
    } else {
        std::for_each(first, last, std::forward<Fn>(fn));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_LOOPS_H
