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

#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/envSetting.h"

#include <tbb/atomic.h>
#include <tbb/task_scheduler_init.h>

#include <algorithm>

// The environment variable used to limit the number of threads the application
// may spawn:
//           0 - maximum concurrency
//           1 - single-threaded mode
//  positive n - limit to n threads (clamped to the number of machine cores)
//  negative n - limit to all but n machine cores (minimum 1).
//
TF_DEFINE_ENV_SETTING(
    PXR_WORK_THREAD_LIMIT, 0,
    "Limits the number of threads the application may spawn. 0 (default) "
    "allows for maximum concurrency.");

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
WorkGetMaximumConcurrencyLimit()
{
    // Use TBB here, since it pays attention to the affinity mask on Linux and
    // Windows.
    return tbb::task_scheduler_init::default_num_threads();
}

// This function always returns an actual thread count >= 1.
static unsigned
Work_NormalizeThreadCount(const int n)
{
    // Zero means use all physical cores available.
    if (n == 0)
        return WorkGetMaximumConcurrencyLimit();

    // One means use exactly one.
    if (n == 1)
        return n;

    // Clamp positive integers to the total number of available cores.
    if (n > 1)
        return std::min<int>(n, WorkGetMaximumConcurrencyLimit());

    // For negative integers, subtract the absolute value from the total number
    // of available cores (denoting all but n cores). If n == number of cores,
    // clamp to 1 to set single-threaded mode.
    return std::max<int>(1, n + WorkGetMaximumConcurrencyLimit());
}

static void 
Work_InitializeThreading()
{
    // Threading is initialized with the value from the environment
    // setting. The setting defaults to 0, i.e. maximum concurrency.
    int settingVal = TfGetEnvSetting(PXR_WORK_THREAD_LIMIT);
    _threadLimit = Work_NormalizeThreadCount(settingVal);
    
    // Only eagerly grab TBB if the PXR_WORK_THREAD_LIMIT setting was set to
    // some non-zero value.
    if (settingVal)
        _tbbTaskSchedInit = new tbb::task_scheduler_init(_threadLimit);
}
static int _forceInitialization = (Work_InitializeThreading(), 0);

void
WorkSetConcurrencyLimit(unsigned n)
{
    // sanity
    _threadLimit = std::max<unsigned>(1, n);

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
    WorkSetConcurrencyLimit(WorkGetMaximumConcurrencyLimit());
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
