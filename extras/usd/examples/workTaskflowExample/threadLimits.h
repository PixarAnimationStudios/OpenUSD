//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_THREAD_LIMITS_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_THREAD_LIMITS_H

#include <thread>

/// Taskflow Thread Limit Implementations

static int _threadLimit = 0;

/// Implements WorkGetPhysicalConcurrencyLimit
inline unsigned
WorkImpl_GetPhysicalConcurrencyLimit()
{
    return std::thread::hardware_concurrency();
}

/// Helps implement Work_InitializeThreading
inline void 
WorkImpl_InitializeThreading(int threadLimit)
{
    return;
}

/// Implements WorkSetConcurrencyLimit
inline void
WorkImpl_SetConcurrencyLimit(int threadLimit)
{
    int numCores = static_cast<int>(std::thread::hardware_concurrency());
    if (threadLimit == 1 || threadLimit < -numCores) {
        _threadLimit = 1;
    } else {
        _threadLimit = numCores;
    }
}

// Implements WorkGetConcurrencyLimit
inline int WorkImpl_GetConcurrencyLimit()
{
    return _threadLimit;
}

/// Implements SupportsGranularThreadLimits
inline bool
WorkImpl_SupportsGranularThreadLimits()
{
    return false;
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_THREAD_LIMITS_H