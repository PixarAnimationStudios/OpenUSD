//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_TEST_ARCH_UTIL_H
#define PXR_BASE_ARCH_TEST_ARCH_UTIL_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

// Crash types.
enum class ArchTestCrashMode {
    Error,
    ReadInvalidAddresses,
    ReadInvalidAddressesWithThread
};

// Cause the test to crash deliberately.
void ArchTestCrash(ArchTestCrashMode mode);

// On Windows we can't easily fork() so we just run the test again with
// command line arguments to request a crash.
void ArchTestCrashArgParse(int argc, char** argv);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_TEST_ARCH_UTIL_H
