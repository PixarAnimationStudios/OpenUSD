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
#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstring>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(ARCH_OS_WINDOWS)
static const char* crashArgument[] = {
    "--crash-raise",
    "--crash-mem",
    "--crash-mem-thread"
};
#endif

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

/*
 * Arch_CorruptMemory
 *     causes the calling program to crash by doing bad malloc things, so
 *     that crash handling behavior can be tested.  If 'spawnthread'
 *     is true, it spawns a thread which is alive during the crash.  If the
 *     program fails to crash, this aborts (since memory will be trashed.)
 */
void
Arch_CorruptMemory(bool spawnthread)
{
    char *overwrite, *another;

    std::thread t;
    if (spawnthread) {
        t = std::thread([](){ while(true) ; });
    }

    for (size_t i = 0; i < 15; ++i) {
        overwrite = (char *)malloc(2);
        another = (char *)malloc(7);

#define STRING "this is a long string, which will overwrite a lot of memory"
        for (size_t j = 0; j <= i; ++j)
            strcpy(overwrite + (j * sizeof(STRING)), STRING);
        cerr << "succeeded in overwriting buffer with sprintf\n";

        free(another);
        cerr << "succeeded in freeing another allocated buffer\n";

        another = (char *)malloc(7);
        cerr << "succeeded in allocating another buffer after overwrite\n";

        another = (char *)malloc(13);
        cerr << "succeeded in allocating a second buffer after overwrite\n";

        another = (char *)malloc(7);
        cerr << "succeeded in allocating a third buffer after overwrite\n";

        free(overwrite);
        cerr << "succeeded in freeing overwritten buffer\n";
        free(overwrite);
        cerr << "succeeded in freeing overwrite AGAIN\n";
    }

    // Added this to get the test to crash with SmartHeap.  
    overwrite = (char *)malloc(1);
    for (size_t i = 0; i < 1000000; ++i)
    overwrite[i] = ' ';

    // Boy, darwin just doesn't want to crash: ok, handle *this*...
    for (size_t i = 0; i < 128000; i++) {
        char* ptr = (char*) malloc(i);
        free(ptr + i);
        free(ptr - i);
        free(ptr);
    }

    cerr << "FAILED to crash! Aborting.\n";
    ArchAbort();
}

void
Arch_TestCrash(ArchTestCrashMode mode)
{
    switch (mode) {
    case ArchTestCrashMode::Error:
        ARCH_ERROR("Testing ArchError");
        break;

    case ArchTestCrashMode::CorruptMemory:
        Arch_CorruptMemory(false);
        break;

    case ArchTestCrashMode::CorruptMemoryWithThread:
        Arch_CorruptMemory(true);
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
        '"' + ArchGetExecutablePath() + '"' +
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

    ARCH_AXIOM(status != 0);
}

void
ArchTestCrashArgParse(int argc, char** argv)
{
#if defined(ARCH_OS_WINDOWS)
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
#else
    // Non-windows platforms don't need this.
#endif
}
