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
// threadLimits.cpp
//

#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/envSetting.h"

#include <tbb/atomic.h>
#include <tbb/task_scheduler_init.h>

#include <algorithm>

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

// This is Work's notion of the currently requested thread limit.  Due to TBB's
// behavior, the first client to create a tbb::task_scheduler_init will
// establish TBB's global default limit.  We only do this as eagerly as possible
// if PXR_WORK_THREAD_LIMIT is set to some nonzero value, otherwise we leave it
// up to others.  So there's no guarantee that calling
// WorkSetConcurrencyLimit(n) will actually limit Work to n threads.
static tbb::atomic<unsigned> _threadLimit;

// We create a task_scheduler_init instance at static initialization time if
// PXR_WORK_THREAD_LIMIT is set to a nonzero value.  Otherwise this stays NULL.
static tbb::task_scheduler_init *_tbbTaskSchedInit;

unsigned
WorkGetPhysicalConcurrencyLimit()
{
    // Use TBB here, since it pays attention to the affinity mask on Linux and
    // Windows.
    return tbb::task_scheduler_init::default_num_threads();
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
    _threadLimit = Work_OverrideConcurrencyLimit(physicalLimit, settingVal);

    // Only eagerly grab TBB if the PXR_WORK_THREAD_LIMIT setting was set to
    // some non-zero value. Otherwise, the scheduler will be default initialized
    // with maximum physical concurrency, or will be left untouched if
    // previously initialized by the hosting environment (e.g. if we are running
    // as a plugin to another application.)
    if (settingVal)
        _tbbTaskSchedInit = new tbb::task_scheduler_init(_threadLimit);
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
    if (n) {
        // Get the thread limit from the environment setting. Note this value
        // may be 0 (default).
        const unsigned settingVal = Work_GetConcurrencyLimitSetting();

        // Override n with the environment setting. This will make sure that the
        // setting always wins over the specified value n, but only if the
        // setting has been set to a non-zero value.
        _threadLimit = Work_OverrideConcurrencyLimit(n, settingVal);
    }

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
        _tbbTaskSchedInit->initialize(_threadLimit);
    } else {
        _tbbTaskSchedInit = new tbb::task_scheduler_init(_threadLimit);
    }
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
    return _threadLimit;
}

PXR_NAMESPACE_CLOSE_SCOPE
