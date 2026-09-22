//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/semaphore.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiSemaphore::HgiSemaphore(HgiSemaphoreKind kind)
    : _kind(kind)
{
}

HgiSemaphore::~HgiSemaphore() = default;

PXR_NAMESPACE_CLOSE_SCOPE
