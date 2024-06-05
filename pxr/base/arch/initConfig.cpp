//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/systemInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

void Arch_InitDebuggerAttach();
void Arch_InitTmpDir();
void Arch_SetAppLaunchTime();
void Arch_ValidateAssumptions();
void Arch_InitTickTimer();

namespace {

ARCH_CONSTRUCTOR(Arch_InitConfig, 2, void)
{
    // Initialize the application start time.  First so it's a close as
    // possible to the real start time.
    Arch_SetAppLaunchTime();

    // Initialize the temp directory.  Early so other initialization
    // functions can use it.
    Arch_InitTmpDir();

    // Initialize program name for errors.  Early for initialization
    // error reporting.
    ArchSetProgramNameForErrors(ArchGetExecutablePath().c_str());

    // Perform platform validations: these are very quick, lightweight
    // checks.  The reason that we call this function here is that pretty
    // much any program that uses anything from lib/tf will end up here
    // at some point.  It is not so important that *every* program
    // perform this check; what is important is that when we bring up a new
    // architecture/compiler/build, the validation gets performed at some
    // point, to alert us to any problems.
    Arch_ValidateAssumptions();

    // Initialize the debugger interface.
    Arch_InitDebuggerAttach();
}

}

PXR_NAMESPACE_CLOSE_SCOPE

