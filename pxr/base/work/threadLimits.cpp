//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// threadLimits.cpp
//

#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/envSetting.h"

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

PXR_NAMESPACE_USING_DIRECTIVE

// The environment variable used to limit the number of threads the application
// may spawn:
//           0 - no change, i.e. defaults to maximum physical concurrency
//           1 - single-threaded mode
//  positive n - limit to n threads
//  negative n - limit to all but n machine cores (minimum 1).
//
// Note that the environment variable value always wins over any value passed to
// the API calls below. If PXR_WORK_THREAD_LIMIT is set to a non-zero value, the
// concurrency limit cannot be changed at runtime.
//
TF_DEFINE_ENV_SETTING(
    PXR_WORK_THREAD_LIMIT, 0,
    "Limits the number of threads the application may spawn. 0 (default) "
    "allows for maximum concurrency as determined by the number of physical "
    "cores, or the process's affinity mask, whichever is smaller. Note that "
    "the environment variable (if set to a non-zero value) will override any "
    "value passed to Work thread-limiting API calls.");

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
WorkGetPhysicalConcurrencyLimit()
{
    // Use TBB here, since it pays attention to the affinity mask on Linux and
    // Windows.
#if TBB_INTERFACE_VERSION_MAJOR >= 12
    return tbb::info::default_concurrency();
#else
    return tbb::task_scheduler_init::default_num_threads();
#endif
}

// This function always returns an actual thread count >= 1.
static unsigned
Work_NormalizeThreadCount(const int n)
{
    // Zero means "no change", and n >= 1 means exactly n threads, so simply
    // pass those values through unchanged.
    // For negative integers, subtract the absolute value from the total number
    // of available cores (denoting all but n cores). If n == number of cores,
    // clamp to 1 to set single-threaded mode.
    return n >= 0 ? n : std::max<int>(1, n + WorkGetPhysicalConcurrencyLimit());
}

// Returns the normalized thread limit value from the environment setting. Note
// that 0 means "no change", i.e. the environment setting does not apply.
static unsigned
Work_GetConcurrencyLimitSetting()
{
    return Work_NormalizeThreadCount(TfGetEnvSetting(PXR_WORK_THREAD_LIMIT));
}

// Overrides weakValue with strongValue if strongValue is non-zero, and returns
// the resulting thread limit.
static unsigned
Work_OverrideConcurrencyLimit(unsigned weakValue, unsigned strongValue)
{
    // If the new limit is 0, i.e. "no change", simply pass the weakValue
    // through unchanged. Otherwise, the new value wins.
    return strongValue ? strongValue : weakValue;
}

static void 
Work_InitializeThreading()
{
    // Get the thread limit from the environment setting. Note that this value
    // can be 0, i.e. the environment setting does not apply.
    const unsigned settingVal = Work_GetConcurrencyLimitSetting();

    // Threading is initialized with maximum physical concurrency.
    const unsigned physicalLimit = WorkGetPhysicalConcurrencyLimit();

    // To assign the thread limit, override the initial limit with the
    // environment setting. The environment setting always wins over the initial
    // limit, unless it has been set to 0 (default). Semantically, 0 means
    // "no change".
    unsigned threadLimit =
        Work_OverrideConcurrencyLimit(physicalLimit, settingVal);

    // Only eagerly grab TBB if the PXR_WORK_THREAD_LIMIT setting was set to
    // some non-zero value. Otherwise, the scheduler will be default initialized
    // with maximum physical concurrency, or will be left untouched if
    // previously initialized by the hosting environment (e.g. if we are running
    // as a plugin to another application.)
    if (settingVal) {
#if TBB_INTERFACE_VERSION_MAJOR >= 12
        _tbbGlobalControl = new tbb::global_control(
            tbb::global_control::max_allowed_parallelism, threadLimit);
#else
        _tbbTaskSchedInit = new tbb::task_scheduler_init(threadLimit);
#endif
    }
}
static int _forceInitialization = (Work_InitializeThreading(), 0);

void
WorkSetConcurrencyLimit(unsigned n)
{
    // We only assign a new concurrency limit if n is non-zero, since 0 means
    // "no change". Note that we need to re-initialize the TBB
    // task_scheduler_init instance in either case, because if the client
    // explicitly requests a concurrency limit through this library, we need to
    // attempt to take control of the TBB scheduler if we can, i.e. if the host
    // environment has not already done so.
    unsigned threadLimit = 0;
    if (n) {
        // Get the thread limit from the environment setting. Note this value
        // may be 0 (default).
        const unsigned settingVal = Work_GetConcurrencyLimitSetting();

        // Override n with the environment setting. This will make sure that the
        // setting always wins over the specified value n, but only if the
        // setting has been set to a non-zero value.
        threadLimit = Work_OverrideConcurrencyLimit(n, settingVal);
    }
    else {
        // Use the current thread limit.
        threadLimit = WorkGetConcurrencyLimit();
    }

    
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

void 
WorkSetMaximumConcurrencyLimit()
{
    WorkSetConcurrencyLimit(WorkGetPhysicalConcurrencyLimit());
}

void
WorkSetConcurrencyLimitArgument(int n)
{
    WorkSetConcurrencyLimit(Work_NormalizeThreadCount(n));
}

unsigned
WorkGetConcurrencyLimit()
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
WorkHasConcurrency()
{
    return WorkGetConcurrencyLimit() > 1;
}

PXR_NAMESPACE_CLOSE_SCOPE
