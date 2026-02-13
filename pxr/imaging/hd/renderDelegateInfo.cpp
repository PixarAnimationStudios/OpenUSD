//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderDelegateInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

bool operator==(const HdRenderDelegateInfo &a, const HdRenderDelegateInfo &b)
{
    return
        a.materialBindingPurpose == b.materialBindingPurpose &&
        a.materialRenderContexts == b.materialRenderContexts &&
        a.renderSettingsNamespaces == b.renderSettingsNamespaces &&
        a.isPrimvarFilteringNeeded == b.isPrimvarFilteringNeeded &&
        a.shaderSourceTypes == b.shaderSourceTypes &&
        a.isCoordSysSupported == b.isCoordSysSupported;
}

bool operator!=(const HdRenderDelegateInfo &a, const HdRenderDelegateInfo &b)
{
    return !(a==b);
}

PXR_NAMESPACE_CLOSE_SCOPE
