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
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/defines.h"

#include <string>
#include <stdlib.h>
#include <sys/types.h>
#if defined(ARCH_OS_WINDOWS)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#include <csignal>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <assert.h>

void
crash(int sig) {
        printf("crashed!\n");
        exit(sig);
}

void crashCallback(void* data) {
    assert( data );
}

int main()
{
    ArchSetProgramNameForErrors( "testArch ArchError" );

    (void) signal(SIGABRT,crash);
        
    std::string log = ArchMakeTmpFileName("statusLogTester");
    FILE *logFile = nullptr;
    int childPid;
    int status;

    assert((logFile = ArchOpenFile(log.c_str(), "w")) != NULL);
    fputs("fake log\n", logFile);
    fputs("let's throw in a weird printf %1024$s specifier\n", logFile);
    fclose(logFile);
    

    ArchLogStackTrace("Crashing", true, log.c_str());
    ArchUnlinkFile(log.c_str());

    ArchLogPostMortem("Test Crashing");
#if defined(ARCH_OS_WINDOWS)
    status = 0;
    childPid = 0;
#else
    // test crashing with and without spawning
    if ( (childPid = fork()) == 0 )   {
        printf("Crash (don't spawn thread)\n");
        ArchTestCrash(false);
        exit(0);
    }


    assert(childPid == wait(&status));
    assert(status != 0);

    if ( (childPid = fork()) == 0 )   {
        printf("Crash (spawn thread)\n");
        ArchTestCrash(true);
        exit(0);
    }

    assert(childPid == wait(&status));
    assert(status != 0);
#endif

    // test GetStackTrace
    std::vector<std::string> stackTrace = ArchGetStackTrace(20);
    bool found = false;
    for (unsigned int i = 0; i < stackTrace.size(); i++) {
        found |= (stackTrace[i].find("main", 0) != std::string::npos);
    }
#if defined(ARCH_OS_DARWIN)
    // We don't dump stack traces on Darwin yet, so we can never
    // find main() on the stack. When we fix Bug 431 and enable
    // stack traces on Darwin, this test will fail and the fix
    // will be to simply remove this #if code.
    found = !found;
#endif
    assert(found);

    return 0;
}

