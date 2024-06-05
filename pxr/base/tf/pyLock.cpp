//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/diagnosticLite.h"

PXR_NAMESPACE_OPEN_SCOPE

TfPyLock::TfPyLock()
    : _acquired(false)
    , _allowingThreads(false)
{
    // Acquire the lock on construction
    Acquire();
}

TfPyLock::TfPyLock(_UnlockedTag)
    : _acquired(false)
    , _allowingThreads(false)
{
    // Do not acquire the lock.
}

TfPyLock::~TfPyLock()
{
    // Restore thread state if it's been saved to allow other threads.
    if (_allowingThreads)
        EndAllowThreads();

    // Release the lock if we have it
    if (_acquired)
        Release();
}

void
TfPyLock::Acquire()
{
    // If already acquired, emit a warning and do nothing
    if (_acquired) {
        TF_WARN("Cannot recursively acquire a TfPyLock.");
        return;
    }

    if (!Py_IsInitialized())
        return;

    // Acquire the GIL and swap in our thread state
    _gilState = PyGILState_Ensure();
    _acquired = true;
}

void
TfPyLock::Release()
{
    // If not acquired, emit a warning and do nothing
    if (!_acquired) {
        if (Py_IsInitialized())
            TF_WARN("Cannot release a TfPyLock that is not acquired.\n");
        return;
    }

    // If allowing threads, emit a warning and do nothing
    if (_allowingThreads) {
        TF_WARN("Cannot release a TfPyLock that is allowing threads.\n");
        return;
    }

    // Release the GIL and restore the previous thread state
    PyGILState_Release(_gilState);
    _acquired = false;
}

void
TfPyLock::BeginAllowThreads()
{
    // If already allowing threads, emit a warning and do nothing
    if (_allowingThreads) {
        TF_WARN("Cannot recursively allow threads on a TfPyLock.\n");
        return;
    }

    // If not acquired, emit a warning and do nothing
    if (!_acquired) {
        if (Py_IsInitialized())
            TF_WARN("Cannot allow threads on a TfPyLock that is not "
                    "acquired.\n");
        return;
    }

    // Save the thread state locally.
    _savedState = PyEval_SaveThread();
    _allowingThreads = true;
}

void
TfPyLock::EndAllowThreads()
{
    // If not allowing threads, emit a warning and do nothing
    if (!_allowingThreads) {
        TF_WARN("Cannot end allowing threads on a TfPyLock that is not "
                "currently allowing threads.\n");
        return;
    }

    PyEval_RestoreThread(_savedState);
    _allowingThreads = false;
}

TfPyEnsureGILUnlockedObj::TfPyEnsureGILUnlockedObj()
    : _lock(TfPyLock::_ConstructUnlocked)
{
    if (PyGILState_Check()) {
        _lock.Acquire();
        _lock.BeginAllowThreads();
    }        
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_PYTHON_SUPPORT_ENABLED
