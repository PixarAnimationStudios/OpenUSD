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
// Written by dhl (10 Jul 2006)
//

#include "pxr/base/arch/error.h"

#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <csignal>

void
crash(int sig) {
        printf("crashed!\n");
        exit(sig);
}

int main()
{
    (void) signal(SIGABRT,crash);

#if !defined(ARCH_OS_WINDOWS)
    int childPid;

    if ( (childPid = fork()) == 0 )   {
        printf("Should print error message:\n");
        ARCH_ERROR("TESTING ARCH ERROR");
        exit(0);
    }
    int status;

    assert(childPid == wait(&status));
#endif
    assert(status != 0);

    return 0;
}
