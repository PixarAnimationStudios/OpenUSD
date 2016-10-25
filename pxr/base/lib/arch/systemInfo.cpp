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
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/stackTrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/error.h"
#if defined(ARCH_OS_DARWIN)
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#endif

#ifdef _POSIX_VERSION
#include <limits.h>                     /* for PATH_MAX */
#else
#include <sys/param.h>                  /* for MAXPATHLEN */
#endif

#ifndef PATH_MAX
#ifdef _POSIX_VERSION
#define PATH_MAX _POSIX_PATH_MAX
#else
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

using std::string;

string
ArchGetCwd()
{
    char space[4096];
    if (getcwd(space, 4096))
	return string(space);


#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

    /*
     * Try dynamic sizing.
     */

# if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    char* buf = getcwd(NULL, 0);
# else
    char* buf = getcwd(NULL, -1);
# endif    

    if (buf) {
	string result(buf);
	free(buf);
	return result;
    }

#else
#error Unknown architecture.
#endif    

    ARCH_WARNING("can't determine working directory");
    return ".";
}

string
ArchGetExecutablePath()
{
#if defined(ARCH_OS_LINUX) 
    // Under Linux, the command line is retrieved from /proc/<pid>/cmdline.
    // Open the file
    char buf[2048];
    int len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1) {
        ARCH_WARNING("Unable to read /proc/self/exe to obtain executable path");
        len = 0;
    }

    buf[len] = '\0';
    return buf;
#elif defined(ARCH_OS_DARWIN)
    char linebuffer[1024];
    uint32_t bufsize = 1024;
    linebuffer[0] = 0;
    _NSGetExecutablePath(linebuffer, &bufsize);
    return linebuffer;
#else

#error Unknown architecture.    

#endif
}
