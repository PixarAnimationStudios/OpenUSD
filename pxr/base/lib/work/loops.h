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

///
///\file work/loops.h

#include <boost/function.hpp>
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include <tbb/tbb.h>

#include "pxr/base/work/api.h"



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
void WORK_API 
WorkParallelForN(
    size_t n, 
    const boost::function< void (size_t begin, size_t end) > &callback);


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
    tbb::parallel_for_each(first, last, fn, ctx);
}

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
template<typename T>
void
WorkSerialForN(size_t n, const T &fn)
{
    fn(0, n);
}

#endif
