//
// Copyright 2018 Pixar
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
#ifndef WORK_REDUCE_H
#define WORK_REDUCE_H

/// \file work/reduce.h
#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/work/api.h"

#include <tbb/tbb.h>

PXR_NAMESPACE_OPEN_SCOPE


///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelReduceN(size_t n, CallbackType callback,
///     ReductionType reduction, size_t grainSize)
///
/// Runs \p reduction in parallel over the range 0 to n
///
/// Callback must be of the form:
///
///     T LoopCallback(size_t begin, size_t end, V value);
///
/// Reduction must be of the form:
///
///     T Reduction(T a, T b)
///
/// grainSize specifices a minumum amount of work to be done per-thread. There
/// is overhead to launching a thread (or task) and a typical guideline is that
/// you want to have at least 10,000 instructions to count for the overhead of
/// launching a thread.
///
template <typename Fn, typename Rn, typename V>
V
WorkParallelReduceN(V value,
    size_t n,
    Fn &&callback,
    Rn &&reduction,
    size_t grainSize)
{
    if (n == 0)
        return value;

    // Don't bother with parallel_reduce, if concurrency is limited to 1.
    if (WorkGetConcurrencyLimit() > 1) {

        class Work_Body_TBB
        {
        public:
            Work_Body_TBB(Fn &fn) : _fn(fn) { }

            V operator()(const tbb::blocked_range<size_t> &r, V value) const {
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

        class Work_Reduce_TBB
        {
        public:
            Work_Reduce_TBB(Rn &rn) : _rn(rn) { }

            V operator()(V lhs, V rhs) const {
                // Note that we std::forward _fn using Fn in order get the
                // right operator().
                // We maintain the right type in this way:
                //  If Fn is T&, then reference collapsing gives us T& for _fn
                //  If Fn is T, then std::forward correctly gives us T&& for _fn
                return std::forward<Rn>(_rn)(lhs, rhs);
            }
        private:
            Rn &_rn;
        };

        // In most cases we do not want to inherit cancellation state from the
        // parent context, so we create an isolated task group context.
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        value = tbb::parallel_reduce(tbb::blocked_range<size_t>(0,n,grainSize),
            value,
            Work_Body_TBB(callback),
            Work_Reduce_TBB(reduction),
            tbb::auto_partitioner(),
            ctx);
    } else {
        // If concurrency is limited to 1, execute serially.
        value = std::forward<Fn>(callback)(0, n, value);
    }
    return value;
}

///////////////////////////////////////////////////////////////////////////////
///
/// WorkParallelReduceN(size_t n, CallbackType callback,
///     ReductionType reduction)
///
/// Runs \p reduction in parallel over the range 0 to n
///
/// Callback must be of the form:
///
///     T LoopCallback(size_t begin, size_t end, V value);
///
/// Reduction must be of the form:
///
///     T Reduction(T a, T b)
///
///
template <typename Fn, typename Rn, typename V>
V
WorkParallelReduceN(V value,
    size_t n,
    Fn &&callback,
    Rn &&reduction)
{
    return WorkParallelReduceN(value, n, callback, reduction, 1);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // WORK_REDUCE_H
