//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_DELEGATE_INFO_H
#define PXR_IMAGING_HD_RENDER_DELEGATE_INFO_H

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;

struct HdRenderDelegateInfo
{
    TfToken materialBindingPurpose;
    TfTokenVector materialRenderContexts;
    TfTokenVector renderSettingsNamespaces;
    bool isPrimvarFilteringNeeded;
    TfTokenVector shaderSourceTypes;
    bool isCoordSysSupported;
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif
