//
// Copyright 2017 Pixar
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
#define _CRT_SECURE_NO_WARNINGS

#include "pxr/pxr.h"
#include "pxr/base/arch/testArchUtil.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/systemInfo.h"
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <process.h>
#include <csignal>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(ARCH_OS_WINDOWS)
static const char* crashArgument[] = {
    "--crash-raise",
    "--crash-invalid-read",
    "--crash-invalid-read-thread"
};
#endif

using namespace std;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*
 * Arch_ReadInvalidAddresses
 *     causes the calling program to crash by reading from bad addresses, so
 *     that crash handling behavior can be tested.  If 'spawnthread'
 *     is true, it spawns a thread which is alive during the crash.  If the
 *     program fails to crash, this aborts.
 */
void
Arch_ReadInvalidAddresses(bool spawnthread)
{
    std::thread t;
    if (spawnthread) {
        t = std::thread([](){ while(true) ; });
    }

#if defined(ARCH_OS_WINDOWS)
    // On Windows we simply raise SIGSEGV.  Reading invalid addresses causes the
    // program to terminate, but with a zero return code, which is not what we
    // need for testing purposes here.  I spent a few minutes going down the
    // Windows SEH rabbit hole, but there's no local "quick fix" that will let
    // the pieces plug together. Frankly, if we wnat to support the kind of
    // full-featured postmortem crash reporting we have on Linux, it's going to
    // take a lot of work.  If we ever care to do that, we'll need to revisit
    // this.
    raise(SIGSEGV);
#endif

    for (size_t i = 0; i != ~0ull; ++i) {
        // This will eventually give us NULL in a way that the compiler probably
        // cannot prove at compile-time.
        char const *ptr = reinterpret_cast<char const *>(rand() & 7);
        printf("byte %p = %d\n", ptr, *ptr);
    }

    fprintf(stderr, "FAILED to crash! Aborting.\n");
    ArchAbort();
}

void
Arch_TestCrash(ArchTestCrashMode mode)
{
    switch (mode) {
    case ArchTestCrashMode::Error:
        ARCH_ERROR("Testing ArchError");
        break;

    case ArchTestCrashMode::ReadInvalidAddresses:
        Arch_ReadInvalidAddresses(false);
        break;

    case ArchTestCrashMode::ReadInvalidAddressesWithThread:
        Arch_ReadInvalidAddresses(true);
        break;
    }
}

} // anonymous namespace

void
ArchTestCrash(ArchTestCrashMode mode)
{
    int status;

#if defined(ARCH_OS_WINDOWS)

    // Make a command line for a new copy of this program with an argument
    // to tell it to crash.
    std::string cmdLine =
        '"' + ArchGetExecutablePath() + "\" " +
        crashArgument[static_cast<int>(mode)];

    // Start a new copy of this program and tell it to crash.
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));
    if (!CreateProcess(NULL, const_cast<char*>(cmdLine.c_str()),
                       NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
                       &startupInfo, &processInfo)) {
        ARCH_WARNING("Failed to fork to test a crash");
        _exit(1);
    }

    // Wait for the process to exit.
    DWORD exitCode;
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    status = exitCode;
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

#else

    // Fork and crash in the child.
    int childPid;
    if ( (childPid = fork()) == 0 )   {
        Arch_TestCrash(mode);
        _exit(0);
    }
    else if (childPid == -1) {
        ARCH_WARNING("Failed to fork to test a crash");
        _exit(1);
    }

    // Wait for the child.
    ARCH_AXIOM(childPid == wait(&status));

#endif

    // We reserve status 0 for the child executing without error and
    // status 1 for it having an unexpected error.  Since we expect
    // the child to fail we expect a status greater than 1.  Raising
    // a signal with a default handler on Windows exits with status
    // code 3, a fact we take advantage of in this test.
    ARCH_AXIOM(status > 1);
}

#if defined(ARCH_OS_WINDOWS)
void
ArchTestCrashArgParse(int argc, char** argv)
{
    // Scan for crash argument.
    for (int i = 1; i != argc; ++i) {
        for (size_t j = 0, n = sizeof(crashArgument) / sizeof(crashArgument[0]);
                j != n; ++j) {
            if (strcmp(argv[i], crashArgument[j]) == 0) {
                Arch_TestCrash(static_cast<ArchTestCrashMode>(j));
                _exit(1);
            }
        }
    }
}
#else
void
ArchTestCrashArgParse(int /*argc*/, char** /*argv*/)
{
    // Non-windows platforms don't need this.
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
