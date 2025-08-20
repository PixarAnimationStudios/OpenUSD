//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_API_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_API_H

#if defined(_WIN32)
    #if defined(WORK_TASKFLOW_EXAMPLE_EXPORT)
        #define WORK_TASKFLOW_EXAMPLE_API __declspec(dllexport)
    #else
        #define WORK_TASKFLOW_EXAMPLE_API __declspec(dllimport)
    #endif
#else
    #define WORK_TASKFLOW_EXAMPLE_API
#endif

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_API_H
