//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/threads.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

// Static initializer to get the main thread id.  We want this to run as early
// as possible, so we actually capture the main thread's id.  We assume that
// we're not starting threads before main().

namespace {

const std::thread::id _mainThreadId = std::this_thread::get_id();

} // anonymous namespace

bool ArchIsMainThread()
{
    return std::this_thread::get_id() == _mainThreadId;
}

std::thread::id
ArchGetMainThreadId()
{
    return _mainThreadId;
}

PXR_NAMESPACE_CLOSE_SCOPE
