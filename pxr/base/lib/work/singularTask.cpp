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
#include "pxr/base/work/singularTask.h"

#include "pxr/base/work/dispatcher.h"

struct WorkSingularTask::_Invoker
{
    explicit _Invoker(WorkSingularTask *task) : _task(task) {}

    void operator()() const {
        // We read the current refCount into oldCount, then we invoke the task
        // function.  Finally we try to CAS the refCount to zero.  If we fail,
        // it means some other clients have invoked Wake() in the meantime.  In
        // that case we start over to ensure the task can do whatever it was
        // awakened to do.  Once we successfully take the count to zero, we
        // stop.
        size_t oldCount = _task->_refCount;
        do {
            _task->_fn();
        } while (not _task->_refCount.compare_exchange_strong(oldCount, 0));
    }

private:
    WorkSingularTask *_task;
};

void
WorkSingularTask::Wake()
{
    if (++_refCount == 1)
        _dispatcher.Run(_Invoker(this));
}
