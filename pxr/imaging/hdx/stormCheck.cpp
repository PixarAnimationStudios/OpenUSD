//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/stormCheck.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

bool HdxIsStorm(const HdRenderDelegate* delegate)
{
    return dynamic_cast<const HdStRenderDelegate*>(delegate);
}

PXR_NAMESPACE_CLOSE_SCOPE
