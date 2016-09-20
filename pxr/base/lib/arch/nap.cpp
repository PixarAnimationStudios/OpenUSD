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
#include "pxr/base/arch/nap.h"
#include "pxr/base/arch/defines.h"
#include <time.h>
#if defined(ARCH_OS_WINDOWS)
#include <windows.h>
#elif defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include <unistd.h>
#include <sched.h>
#else
#error Unknown architecture.
#endif

void
ArchNap(size_t hundredths)
{
#if defined(ARCH_OS_WINDOWS)
    // The argument to Sleep() should be in milliseconds.
    Sleep((DWORD)(hundredths) * 10);
#elif defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    timespec rec;

    // Convert the specified time into seconds and nanoseconds.
    rec.tv_sec = static_cast<time_t>(hundredths) / 100;
    rec.tv_nsec = (static_cast<time_t>(hundredths) % 100) * 10000000;

    // We have to sleep for at least 1 nanosecond.
    // Note: neither tv_sec and tv_nsec can be negative,
    // because hundredths is unsigned and tv_nsec is big enough to avoid
    // overflow if hundredths == 99.
    
    if (rec.tv_sec == 0 && rec.tv_nsec == 0) {
	rec.tv_sec = 0;
	rec.tv_nsec = 1;
    }
    nanosleep(&rec, 0);
#else
#error Unknown architecture.
#endif
}

void ArchSleep(uint64_t seconds)
{
#if defined(ARCH_OS_WINDOWS)
    Sleep(seconds / 1000);
#else
    sleep(seconds);
#endif
}

int ArchNanoSleep(const struct timespec *req, struct timespec *rem)
{
#if defined(ARCH_OS_WINDOWS)
    HANDLE timer;
    if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
        return 0;

    LARGE_INTEGER sleepTime;
    sleepTime.QuadPart = req->tv_sec * 1000000000 + req->tv_nsec / 100;
    if(!SetWaitableTimer(timer, &sleepTime, 0, NULL, NULL, FALSE))
    {
        CloseHandle(timer);
        return 0;
    }

    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
#else
    return nanosleep(usec);
#endif
}

void
ArchThreadYield()
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    sched_yield();
#elif defined(ARCH_OS_WINDOWS)
    SwitchToThread();
#else
#error Unknown architecture.
#endif
}
