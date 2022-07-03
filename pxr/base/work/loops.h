//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_BASE_WORK_LOOPS_H
#define PXR_BASE_WORK_LOOPS_H

/// \file work/loops.h
#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/work/api.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/task.h>

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
///
template <typename InputIterator, typename Fn>
inline void
WorkParallelForEach(
    InputIterator first, InputIterator last, Fn &&fn)
{
    tbb::task_group_context ctx(tbb::task_group_context::isolated);
    tbb::parallel_for_each(first, last, std::forward<Fn>(fn), ctx);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_LOOPS_H
