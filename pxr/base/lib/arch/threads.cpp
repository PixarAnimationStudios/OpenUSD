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
#include "pxr/base/arch/threads.h"

// Static initializer to get the main thread id.  We want this to run as early
// as possible, so we actually capture the main thread's id.  We assume that
// we're not starting threads before main().

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>

static DWORD _GetMainThreadId()
{
	return GetCurrentThreadId();
}

DWORD _mainThreadId = _GetMainThreadId();
bool ArchIsMainThread()
{
	return GetCurrentThreadId() == _mainThreadId;
}

#else
#include <pthread.h>

static pthread_t _GetMainThreadId()
{
    return pthread_self();
}
pthread_t _mainThreadId = _GetMainThreadId();

bool ArchIsMainThread()
{
    return pthread_equal(pthread_self(), _mainThreadId);
}

#endif