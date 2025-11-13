//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

bool HdxIsStorm(const HdRenderDelegate * const delegate)
{
    return delegate && delegate->RequiresStormTasks();
}

PXR_NAMESPACE_CLOSE_SCOPE
