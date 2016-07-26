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
// loops.cpp
//

#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"

//
// WorkParallelForN implementation for TBB
//
class Work_ParallelForN_TBB 
{
    typedef 
        boost::function< void (size_t begin, size_t end) >
        _CallbackType;

public:

    // Private constructor because no one is allowed to create one of these
    // objects from the outside.
    Work_ParallelForN_TBB(const _CallbackType &fn)
    : _fn(fn)
    {
    }

    void operator()(const tbb::blocked_range<size_t> &r) const {
        _fn(r.begin(), r.end());
    }

private:


    _CallbackType _fn;
};


void
WorkParallelForN(
    size_t n, 
    const boost::function< void (size_t begin, size_t end) > &callback)
{
    if (n == 0)
        return;

    // Don't bother with parallel_for, if concurrency is limited to 1.
    if (WorkGetConcurrencyLimit() > 1) {

        // Note that for the tbb version in the future, we will likely want to 
        // tune the grain size.
        // In most cases we do not want to inherit cancellation state from the
        // parent context, so we create an isolated task group context.
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,n), 
            Work_ParallelForN_TBB(callback), 
            ctx);

    } else {

        // If concurrency is limited to 1, execute serially.
        WorkSerialForN(n, callback);

    }

} 
