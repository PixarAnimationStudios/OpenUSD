//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_LOCK_H
#define PXR_BASE_TF_PY_LOCK_H

#include "pxr/pxr.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/tf/pySafePython.h"

#include "pxr/base/tf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyLock
///
/// Convenience class for accessing the Python Global Interpreter Lock.
///
/// The Python API is not thread-safe.  Accessing the Python API from outside
/// the context of Python execution requires extra care when multiple threads
/// may be present.  There are various schemes for how this should be done,
/// and the conventions have been changing with Python versions through 2.X.
///
/// This class provides a convenient and centralized location for managing the
/// Python Global Interpreter Lock and related Python Thread State.
///
/// The easiest way to use this class is to simply create a local variable in
/// any function that will access the Python API. Upon construction, this will
/// acquire the Python lock and establish the correct thread state for the
/// caller.  Upon exit from the current scope, when the instance is destroyed,
/// the thread state will be restored and the lock will be released.
///
/// \code
/// void MyFunc()
/// {
///     TfPyLock dummy;
///     ...(access Python API)...
/// }
/// \endcode
///
/// If you need to temporarily release the lock during execution, (to perform
/// blocking I/O for example), you can call Release() explicitly, then call
/// Acquire() again to reclaim the lock.
///
/// \code
/// void MyFunc()
/// {
///     TfPyLock pyLock;
///     ...(access Python API)...
///     pyLock.Release();  // let other threads run while we're blocked
///     ...(some blocking I/O or long running operation)...
///     pyLock.Acquire();
///     ...(more access to Python API)...
/// }
/// \endcode
///
/// Note that it IS EXPLICITLY OK to recursively create instances of this
/// class, and thus recursively acquire the GIL and thread state. It is NOT OK
/// to recursively attempt to Acquire() the same instance, that will have no
/// effect and will generate a diagnostic warning.
///
/// This class also provides an exception-safe way to release the GIL
/// temporarily for blocking calls, like Py_BEGIN/END_ALLOW_THREADS in the
/// Python C API.
///
/// \code
/// void MyFunc()
/// {
///     TfPyLock lock;
///     ...(access Python API)...
///     lock.BeginAllowThreads(); // totally unlock the GIL temporarily.
///     ...(some blocking I/O or long running operation)...
///     lock.EndAllowThreads();
///     ...(more access to Python API)...
/// }
/// \endcode
///
/// This looks similar to the above example using \a Release(), but it is
/// different.  The Python lock is recursive, so the call to \a Release() is
/// not guaranteed to actually release the lock, it just releases the deepest
/// lock. In contrast \a BeginAllowThreads() will fully unlock the GIL so that
/// other threads can run temporarily regardless of how many times the lock is
/// recursively taken.
///
/// The valid states and transitions for this class are as follows.
///
/// State           Valid Transitions
/// --------------------------------------------------------------
/// Released        Acquire() -> Acquired
/// Acquired        Release() -> Released, BeginAllowThreads() -> AllowsThreads
/// AllowsThreads   EndAllowThreads() -> Acquired
///
/// Note that upon construction the class is in the Acquired state.  Upon
/// destruction, the class will move to the Released state.
///
/// \warning Instances of this class should only be used as automatic (stack)
/// variables, or in thread local storage. DO NOT create a single instance
/// that could be shared across multiple threads.
///
class TfPyLock {
public:
    /// Acquires the Python GIL and swaps in callers thread state.
    TF_API TfPyLock();

    /// Releases Python GIL and restores prior threads state.
    TF_API ~TfPyLock();

    /// (Re)acquires GIL and thread state, if previously released.
    TF_API void Acquire();

    /// Explicitly releases GIL and thread state.
    TF_API void Release();

    /// Unlock the GIL temporarily to allow other threads to use python.
    /// Typically this is used to unblock threads during operations like
    /// blocking I/O.  The lock must be acquired when called.
    TF_API void BeginAllowThreads();

    /// End allowing other threads, reacquiring the lock state.
    /// \a BeginAllowThreads must have been successfully called first.
    TF_API void EndAllowThreads();

private:
    // Non-acquiring constructor for TfPyEnsureGILUnlockedObj's use.
    friend struct TfPyEnsureGILUnlockedObj;
    enum _UnlockedTag { _ConstructUnlocked };
    explicit TfPyLock(_UnlockedTag);

    PyGILState_STATE _gilState;
    PyThreadState *_savedState;
    bool _acquired:1;
    bool _allowingThreads:1;
};

// Helper class for TF_PY_ALLOW_THREADS_IN_SCOPE()
struct TfPyEnsureGILUnlockedObj
{
    // Do nothing if the current thread does not have the GIL, otherwise unlock
    // the GIL, and relock upon destruction.
    TF_API TfPyEnsureGILUnlockedObj();
private:
    TfPyLock _lock;
};

/// If the current thread of execution has the python GIL, release it,
/// allowing python threads to run, then upon leaving the current scope
/// reacquire the python GIL. Otherwise, do nothing.
/// 
/// For example:
/// \code
/// {
///     TF_PY_ALLOW_THREADS_IN_SCOPE();
///     // ... long running or blocking operation ...
/// }
/// \endcode
///
/// This is functionally similar to the following, except that it does nothing
/// in case the current thread of execution does not have the GIL.
///
/// \code
/// {
///     TfPyLock lock; lock.BeginAllowThreads();
///     // ... long running or blocking operation ...
/// }
/// \endcode
///
/// \hideinitializer
#define TF_PY_ALLOW_THREADS_IN_SCOPE()                  \
    TfPyEnsureGILUnlockedObj __py_lock_allow_threads__

PXR_NAMESPACE_CLOSE_SCOPE

#else 

// When python is disabled, we stub this macro out to nothing.
#define TF_PY_ALLOW_THREADS_IN_SCOPE()

#endif // PXR_PYTHON_SUPPORT_ENABLED

#endif // PXR_BASE_TF_PY_LOCK_H
