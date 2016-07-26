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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/systemInfo.h"

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace {

// Called when we get a SIGUSR2.
static void
_usr2SignalHandler(int /*dummy*/)
{
    // Fast stack trace.
    TfPrintStackTrace(stdout, "received SIGUSR2");

    // Complete post-mortem without logging to the DB.
    const bool wasLogging = ArchGetFatalStackLogging();
    ArchSetFatalStackLogging(false);
    ArchLogPostMortem("received SIGUSR2");
    ArchSetFatalStackLogging(wasLogging);
}

static void
_InstallUsr2StackTraceLogger()
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler   = _usr2SignalHandler;
    act.sa_flags     = SA_RESTART;
    sigaction(SIGUSR2, &act, NULL);
}

ARCH_CONSTRUCTOR(103)
static
void
_PixarInit()
{
    // Install these implicitly.  This function is public so clients can
    // call it at any time if they want to override any previously set
    // handlers.
    TfInstallTerminateAndCrashHandlers();

    // Install a SIGUSR2 handler.
    _InstallUsr2StackTraceLogger();

    // Override process umask to ensure newly created directories and files
    // user and group write permission (002).
    umask(S_IWOTH);
}

}
