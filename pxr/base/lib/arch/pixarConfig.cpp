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
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/vsnprintf.h"

#include <sys/time.h>

using std::string;

// NOTE: 
// This function is implemented here so that it can be used by the 
// initialization code below. It is not static or in an anonymous
// namespace so that it can be called in archext/datetime.cpp. We
// do not include its declaration in any header in arch because
// we don't want to expose this in the open source distribution.
// We need to tag this function with ARCH_API to ensure it's visible
// from outside this library.
ARCH_API 
string
Arch_GetTimezone()
{
    // tzset(3) initializes the 'tzname' and 'timezone' externs. It does
    // so by reading /etc/localtime unless the environment variable 'TZ'
    // is set.
    tzset();

    // Reconstruct the appropriate TZ variable based on the tzname and
    // timezone externs initialized by tzset. The format of TZ is
    // 'std offset dst' (E.g. PST8PDT) as specified in tzset(3).
    long hours = timezone/3600;
    long minutes = abs((timezone/60) - (hours*60));
    return minutes
        ? ArchStringPrintf("%s%ld:%ld%s",
                           tzname[0], hours, minutes, tzname[1])
        : ArchStringPrintf("%s%ld%s",
                           tzname[0], hours, tzname[1]);
}

namespace {

// Post-mortem handling.
static const char* const postMortemCmdDefault = DEF_BASE_SET_PATH "/bin/stacktrace";
static const char* const postMortemArgvDefault[] = {
    "$cmd",
    "$pid",
    "$log",
    "--process-map",
    "--line-numbers",
    nullptr
};

// Session logging to database handling.
static const char* const sessionLogCmdDefault = DEF_BASE_SET_PATH "/bin/logSessionToDb";
static const char* const sessionLogArgvDefault[] = {
    "$cmd",
    "-a", "$prog",
    "-t", "$time",
    "-p", "$pid",
    "-c", "0",
    nullptr
};
static const char* const sessionCrashLogArgvDefault[] = {
    "$cmd",
    "-a", "$prog",
    "-t", "$time",
    "-p", "$pid",
    "-c", "1", "$stack",
    nullptr
};

ARCH_CONSTRUCTOR(103)
static
void
_PixarInit()
{
    // Initialize timezone.  This avoids localtime() and strftime() from
    // repeatedly reading /etc/localtime to initialize the timezone.
#if defined(ARCH_OS_LINUX)
    if (getenv("TZ") == NULL) {
        setenv("TZ", Arch_GetTimezone().c_str(), /* overwrite */ 0);
    }
#endif

    // Initialize post-mortem handler.
    ArchSetPostMortem(postMortemCmdDefault, postMortemArgvDefault);

    // Initialize session logging handler.
    ArchSetLogSession(sessionLogCmdDefault,
                      sessionLogArgvDefault,
                      sessionCrashLogArgvDefault);
}
}
