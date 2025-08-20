//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/work/workTBB/threadLimits_impl.h"

// Blocked range is not used in this file, but this header happens to pull in
// the TBB version header in a way that works in all TBB versions.
#include <tbb/blocked_range.h>
#include <tbb/task_arena.h>

#if TBB_INTERFACE_VERSION_MAJOR >= 12
#include <tbb/global_control.h>
#include <tbb/info.h>
#else
#include <tbb/task_scheduler_init.h>
#endif

#include <algorithm>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

// We create a global_control or task_scheduler_init instance at static
// initialization time if PXR_WORK_THREAD_LIMIT is set to a nonzero value.
// Otherwise this stays NULL.
#if TBB_INTERFACE_VERSION_MAJOR >= 12
static tbb::global_control *_tbbGlobalControl = nullptr;
#else
static tbb::task_scheduler_init *_tbbTaskSchedInit = nullptr;
#endif

unsigned
WorkImpl_GetPhysicalConcurrencyLimit()
{
    // Use TBB here, since it pays attention to the affinity mask on Linux and
    // Windows.
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    return tbb::info::default_concurrency();
#else
    return tbb::task_scheduler_init::default_num_threads();
#endif
}

void 
WorkImpl_InitializeThreading(unsigned threadLimit)
{
    // Only eagerly grab TBB if the PXR_WORK_THREAD_LIMIT setting was set to
    // some non-zero value. Otherwise, the scheduler will be default initialized
    // with maximum physical concurrency, or will be left untouched if
    // previously initialized by the hosting environment (e.g. if we are running
    // as a plugin to another application.)
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    _tbbGlobalControl = new tbb::global_control(
        tbb::global_control::max_allowed_parallelism, threadLimit);
#else
    _tbbTaskSchedInit = new tbb::task_scheduler_init(threadLimit);
#endif
}

void
WorkImpl_SetConcurrencyLimit(unsigned threadLimit)
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    delete _tbbGlobalControl;
    _tbbGlobalControl = new tbb::global_control(
        tbb::global_control::max_allowed_parallelism, threadLimit);
#else
    // Note that we need to do some performance testing and decide if it's
    // better here to simply delete the task_scheduler_init object instead
    // of re-initializing it.  If we decide that it's better to re-initialize
    // it, then we have to make sure that when this library is opened in 
    // an application (e.g., Maya) that already has initialized its own 
    // task_scheduler_init object, that the limits of those are respected.
    // According to the documentation that should be the case, but we should
    // make sure.  If we do decide to delete it, we have to make sure to 
    // note that it has already been initialized.
    if (_tbbTaskSchedInit) {
        _tbbTaskSchedInit->terminate();
        _tbbTaskSchedInit->initialize(threadLimit);
    } else {
        _tbbTaskSchedInit = new tbb::task_scheduler_init(threadLimit);
    }
#endif
}

unsigned
WorkImpl_GetConcurrencyLimit()
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    // The effective concurrency requires taking into account both the
    // task_arena and internal thread pool size set by global_control.
    // https://github.com/oneapi-src/oneTBB/issues/405
    return std::min<unsigned>(
        tbb::global_control::active_value(
            tbb::global_control::max_allowed_parallelism), 
        tbb::this_task_arena::max_concurrency());
#else
    return tbb::this_task_arena::max_concurrency();
#endif
}

bool
WorkImpl_SupportsGranularThreadLimits()
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
