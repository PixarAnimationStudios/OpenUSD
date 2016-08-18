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
#ifndef WORK_LOOPS_H
#define WORK_LOOPS_H

/// \file work/loops.h
#include <utility>
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include <tbb/tbb.h>

#include "pxr/base/work/api.h"



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
/// WorkParallelForN(size_t n, CallbackType callback)
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
    if (n == 0)
        return;

    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkGetConcurrencyLimit() > 1) {

        class Work_ParallelForN_TBB: boost::noncopyable
        {
        public:
            // Private constructor because no one is allowed to create one of
            // these objects from the outside.
            Work_ParallelForN_TBB(Fn &&fn) : _fn(std::forward<Fn>(fn)) { }
            Work_ParallelForN_TBB(const Work_ParallelForN_TBB& rhs):
                _fn(std::move(rhs._fn)) {}

            void operator()(const tbb::blocked_range<size_t> &r) const {
                std::forward<Fn>(_fn)(r.begin(), r.end());
            }

        private:
            Fn &&_fn;
        };

        // Note that for the tbb version in the future, we will likely want to
        // tune the grain size.
        // In most cases we do not want to inherit cancellation state from the
        // parent context, so we create an isolated task group context.
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,n),
            Work_ParallelForN_TBB(std::forward<Fn>(callback)),
            ctx);

    } else {

        // If concurrency is limited to 1, execute serially.
        WorkSerialForN(n, std::forward<Fn>(callback));

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
///
template <typename InputIterator, typename Fn>
inline void
WorkParallelForEach(
    InputIterator first, InputIterator last, Fn &fn)
{
    tbb::task_group_context ctx(tbb::task_group_context::isolated);
    tbb::parallel_for_each(first, last, std::forward<Fn>(fn), ctx);
}

#endif
