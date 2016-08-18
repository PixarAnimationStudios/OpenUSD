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
#ifndef WORK_UTILS_H
#define WORK_UTILS_H

///\file work/utils.h

#include "pxr/base/work/api.h"
#include "pxr/base/work/detachedTask.h"

#include <utility>

WORK_API
bool Work_ShouldSynchronizeAsyncDestroyCalls();

template <class T>
struct Work_AsyncDestroyHelper {
    void operator()() const { /* do nothing */ }
    T obj;
};

/// Swap \p obj with a default-constructed T instance, return and arrange
/// for the swapped-out instance to be destroyed asynchronously.  This means
/// that any code that obj's destructor might invoke must be safe to run both
/// concurrently with other code and at any point in the future.  This might not
/// be true, for example, if obj's destructor might try to update some other
/// data structure that could be destroyed by the time obj's destruction occurs.
/// Be careful.
template <class T>
void WorkSwapDestroyAsync(T &obj)
{
    using std::swap;
    Work_AsyncDestroyHelper<T> helper;
    swap(helper.obj, obj);
    if (!Work_ShouldSynchronizeAsyncDestroyCalls())
        WorkRunDetachedTask(std::move(helper));
}

/// Like WorkSwapDestroyAsync() but instead, move from \p obj, leaving it
/// in a moved-from state instead of a default constructed state.
template <class T>
void WorkMoveDestroyAsync(T &obj)
{
    Work_AsyncDestroyHelper<T> helper { std::move(obj) };
    if (!Work_ShouldSynchronizeAsyncDestroyCalls())
        WorkRunDetachedTask(std::move(helper));
}

#endif // WORK_UTILS_H
