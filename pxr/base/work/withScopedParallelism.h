//
// Copyright 2021 Pixar
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
#ifndef PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H
#define PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H

///\file work/withScopedParallelism.h

#include "pxr/pxr.h"
#include "pxr/base/work/api.h"

#include <tbb/task_arena.h>

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
///
/// This limits parallelism by only a small degree.  It's only the waiting
/// thread that restricts the tasks it can take to the protected scope: all
/// other worker threads continue unhindered.
///
template <class Fn>
void
WorkWithScopedParallelism(Fn &&fn)
{
    tbb::this_task_arena::isolate(std::forward<Fn>(fn));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_WITH_SCOPED_PARALLELISM_H

