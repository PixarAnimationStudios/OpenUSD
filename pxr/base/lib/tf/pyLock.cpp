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

PXR_NAMESPACE_CLOSE_SCOPE

// See https://github.com/pankajp/pygilstate_check
//
PyAPI_DATA(PyThreadState *) _PyThreadState_Current;

PXR_NAMESPACE_OPEN_SCOPE

TfPyEnsureGILUnlockedObj::TfPyEnsureGILUnlockedObj()
    : _lock(TfPyLock::_ConstructUnlocked)
{
    // If we have the python lock, call Acquire() (to get the _lock object into
    // the correct state) and then BeginAllowThreads() to unlock it.  Otherwise
    // do nothing.  In Python 3.4+, this can be replaced by PyGILState_Check().
    PyThreadState *tstate = _PyThreadState_Current;
    if (tstate && (tstate == PyGILState_GetThisThreadState())) {
        _lock.Acquire();
        _lock.BeginAllowThreads();
    }        
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_PYTHON_SUPPORT_ENABLED
