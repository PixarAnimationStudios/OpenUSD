//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H
#define PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H

///\file work/withScopedParallelism.h

#include "pxr/pxr.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/impl.h"
#include "pxr/base/tf/pyLock.h"


#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Invoke \p fn, ensuring that all wait operations on concurrent constructs
/// invoked by the calling thread only take tasks created within the scope of \p
/// fn 's execution.
///
/// Ordinarily when a thread invokes a wait operation on a concurrent construct
/// (e.g. the explicit WorkDispatcher::Wait(), or the implicit wait in loops
/// like WorkParallelForEach()) it joins the pool of worker threads and executes
/// tasks to help complete the work.  This is good, since the calling thread
/// does useful work instead of busy waiting or sleeping until the work has
/// completed.  However, this can be problematic depending on the calling
/// context, and which tasks the waiting thread executes.
///
/// For example, consider the following example: a demand-populated resource
/// cache.
///
/// \code
/// ResourceHandle
/// GetResource(ResourceKey key) {
///     // Attempt to lookup/insert an entry for \p key.  If we insert the
///     // element, then populate the resource.
///     ResourceAccessorAndLock accessor;
///     if (_resources.FindOrCreate(key, &accessor)) {
///         // No previous entry, so populate the resource.
///         WorkDispatcher wd;
///         wd.Run( /* resource population task 1 */);
///         wd.Run( /* resource population task 2 */);
///         wd.Run( /* resource population task 3 */);
///         WorkParallelForN( /* parallel population code */);
///         wd.Wait();
///         /* Store resource data. */
///     }
///     return *accessor.first;
/// }
/// \endcode
///
/// Here when a caller has requested the resource for \p key for the first time,
/// we do the work to populate the resource while holding a lock on that
/// resource entry in the cache.  The problem is that when the calling thread
/// waits for work to complete, if it picks up tasks unrelated to this context
/// and those tasks attempt to call GetResource() with the same key, the process
/// will deadlock.
///
/// This can be fixed by using WorkWithScopedParallelism() to ensure that the
/// calling thread's wait operations only take tasks that were created during
/// the scope of the population work:
///
/// \code
/// ResourceHandle
/// GetResource(ResourceKey key) {
///     // Attempt to lookup/insert an entry for \p key.  If we insert the
///     // element, then populate the resource.
///     ResourceAccessorAndLock accessor;
///     if (_resources.FindOrCreate(key, &accessor)) {
///         // No previous entry, so populate the resource.
///         WorkWithScopedParallelism([&accessor]() {
///             WorkDispatcher wd;
///             wd.Run( /* resource population task 1 */);
///             wd.Run( /* resource population task 2 */);
///             wd.Run( /* resource population task 3 */);
///             WorkParallelForN( /* parallel population code */);
///         }
///         /* Store resource data. */
///     }
///     return *accessor.first;
/// }
/// \endcode
///
/// This limits parallelism by only a small degree.  It's only the waiting
/// thread that restricts the tasks it can take to the protected scope: all
/// other worker threads continue unhindered.
///
/// If Python support is enabled and \p dropPythonGIL is true, this function
/// ensures the GIL is released before invoking \p fn.  If this function
/// released the GIL, it reacquires it before returning.
///
template <class Fn>
auto
WorkWithScopedParallelism(Fn &&fn, bool dropPythonGIL=true)
{
    PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;

    if (dropPythonGIL) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return WorkImpl_WithScopedParallelism(std::forward<Fn>(fn));
    }
    return WorkImpl_WithScopedParallelism(std::forward<Fn>(fn));
}

/// Similar to WorkWithScopedParallelism(), but pass a WorkDispatcher instance
/// to \p fn for its use during the scoped parallelism.  Accordingly, \p fn must
/// accept a WorkDispatcher lvalue reference argument.  After \p fn returns but
/// before the scoped parallelism ends, call WorkDispatcher::Wait() on the
/// dispatcher instance.  The \p dropPythonGIL argument has the same meaning as
/// it does for WorkWithScopedParallelism().
template <class Fn>
auto
WorkWithScopedDispatcher(Fn &&fn, bool dropPythonGIL=true)
{
    return WorkWithScopedParallelism([&fn]() {
        WorkDispatcher dispatcher;
        return std::forward<Fn>(fn)(dispatcher);
        // dispatcher's destructor invokes Wait() here.
    }, dropPythonGIL);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H

