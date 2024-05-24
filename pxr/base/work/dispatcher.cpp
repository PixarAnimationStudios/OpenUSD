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

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"

PXR_NAMESPACE_OPEN_SCOPE

WorkDispatcher::WorkDispatcher()
    : _context(
        tbb::task_group_context::isolated,
        tbb::task_group_context::concurrent_wait | 
        tbb::task_group_context::default_traits)
#if TBB_INTERFACE_VERSION_MAJOR >= 12
      , _taskGroup(_context)
#endif
{
    _waitCleanupFlag.clear();
    
#if TBB_INTERFACE_VERSION_MAJOR < 12
    // The concurrent_wait flag used with the task_group_context ensures
    // the ref count will remain at 1 after all predecessor tasks are
    // completed, so we don't need to keep resetting it in Wait().
    _rootTask = new(tbb::task::allocate_root(_context)) tbb::empty_task;
    _rootTask->set_ref_count(1);
#endif
}

#if TBB_INTERFACE_VERSION_MAJOR >= 12
inline tbb::detail::d1::wait_context& 
_TaskGroup::_GetInternalWaitContext() {
    return m_wait_ctx;
}
#endif

WorkDispatcher::~WorkDispatcher() noexcept
{
    Wait();

#if TBB_INTERFACE_VERSION_MAJOR < 12
    tbb::task::destroy(*_rootTask);
#endif
}

void
WorkDispatcher::Wait()
{
    // Wait for tasks to complete.
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    // The native task_group::wait() has a comment saying its call to the
    // context reset method is not thread safe. So we do our own
    // synchronization to ensure it is called once.
    tbb::detail::d1::wait(_taskGroup._GetInternalWaitContext(), _context);
#else
    _rootTask->wait_for_all();
#endif

    // If we take the flag from false -> true, we do the cleanup.
    if (_waitCleanupFlag.test_and_set() == false) {
        // Reset the context if canceled.
        if (_context.is_group_execution_cancelled()) {
            _context.reset();
        }

        // Post all diagnostics to this thread's list.
        for (auto &et: _errors) {
            et.Post();
        }
        _errors.clear();
        _waitCleanupFlag.clear();
    }
}

void
WorkDispatcher::Cancel()
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    _taskGroup.cancel();
#else
    _context.cancel_group_execution();
#endif
}

/* static */
void
WorkDispatcher::_TransportErrors(const TfErrorMark &mark,
                                 _ErrorTransports *errors)
{
    TfErrorTransport transport = mark.Transport();
    errors->grow_by(1)->swap(transport);
}

PXR_NAMESPACE_CLOSE_SCOPE
