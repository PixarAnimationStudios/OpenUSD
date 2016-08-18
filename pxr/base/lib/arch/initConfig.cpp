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
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/systemInfo.h"

void Arch_InitDebuggerAttach();
void Arch_InitTmpDir();
void Arch_SetAppLaunchTime();
void Arch_ValidateAssumptions();
void Arch_InitTickTimer();

namespace {

ARCH_CONSTRUCTOR_DEFINE(102, Arch_InitConfig)
{
    // Initialize the application start time.  First so it's a close as
    // possible to the real start time.
    Arch_SetAppLaunchTime();

    // Initialize the temp directory.  Early so other initialiation
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

    // Initialize the tick timer.
    Arch_InitTickTimer();

    // Initialize the debugger interface.
    Arch_InitDebuggerAttach();
}

}