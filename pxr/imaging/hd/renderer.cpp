//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderer.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderer::~HdRenderer() = default;

HdLegacyRenderControlInterface *
HdRenderer::GetLegacyRenderControl()
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
