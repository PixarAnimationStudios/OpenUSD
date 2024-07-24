//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/resourceRegistry.h"
#include "hdPrman/renderParam.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_ResourceRegistry::HdPrman_ResourceRegistry(
    std::shared_ptr<HdPrman_RenderParam> const& renderParam)
    : _renderParam(renderParam)
{
}

HdPrman_ResourceRegistry::~HdPrman_ResourceRegistry() = default;

void
HdPrman_ResourceRegistry::ReloadResource(
    TfToken const& resourceType,
    std::string const& path)
{
    if (resourceType == HdResourceTypeTokens->texture) {
        _renderParam->InvalidateTexture(path);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
