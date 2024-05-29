//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/stackTrace.h"

#include <chrono>
#include <iostream>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * This executable performs an invalid memory reference (SIGSEGV)
 * for testing of the Tf crash handler
 */

static void
_ThreadTask()
{
    TfErrorMark m;
    TF_RUNTIME_ERROR("Pending secondary thread error for crash report!");
    std::this_thread::sleep_for(std::chrono::minutes(10));
}

int
main(int argc, char **argv)
{
    ArchSetFatalStackLogging( true );

    // Make sure handlers have been installed
    // This isn't guaranteed in external environments
    // as we leave them off by default.
    TfInstallTerminateAndCrashHandlers();

    TfErrorMark m;

    TF_RUNTIME_ERROR("Pending error to report in crash output!");

    std::thread t(_ThreadTask);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    int* bunk(0);
    std::cout << *bunk << '\n';
}


