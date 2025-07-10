//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"

PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

template <class Impl>
Work_Dispatcher<Impl>::Work_Dispatcher()
    : _isCancelled(false)
{
    _waitCleanupFlag.clear();
}

template <class Impl>
Work_Dispatcher<Impl>::~Work_Dispatcher() noexcept
{
    Wait();
}

template <class Impl>
void
Work_Dispatcher<Impl>::Wait()
{
    // Wait for tasks to complete.
    _dispatcher.Wait();

    // If we take the flag from false -> true, we do the cleanup.
    if (_waitCleanupFlag.test_and_set() == false) {
        _dispatcher.Reset();

        // Post all diagnostics to this thread's list.
        for (auto &et: _errors) {
            et.Post();
        }
        _errors.clear();
        _waitCleanupFlag.clear();
        _isCancelled = false;
    }
}

template <class Impl>
bool
Work_Dispatcher<Impl>::IsCancelled() const
{
    return _isCancelled;
}

template <class Impl>
void
Work_Dispatcher<Impl>::Cancel()
{
    _isCancelled = true;
    _dispatcher.Cancel();
}

/* static */
template <class Impl>
void
Work_Dispatcher<Impl>::_TransportErrors(const TfErrorMark &mark,
                                 _ErrorTransports *errors)
{
    TfErrorTransport transport = mark.Transport();
    errors->grow_by(1)->swap(transport);
}

// Explicitly instantiate Work_Dispatchers
template class Work_Dispatcher<PXR_WORK_IMPL_NS::WorkImpl_Dispatcher>;
#if defined WORK_IMPL_HAS_ISOLATING_DISPATCHER
template class Work_Dispatcher<PXR_WORK_IMPL_NS::WorkImpl_IsolatingDispatcher>;
#endif

PXR_NAMESPACE_CLOSE_SCOPE
