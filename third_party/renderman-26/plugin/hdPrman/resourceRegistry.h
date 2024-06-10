//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RESOURCE_REGISTRY_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RESOURCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// HdPrman's implementation of the hydra resource registry.
/// Renderman manages its resources internally, but uses the HdResourceRegistry
/// to respond to certain resource changes, such as texture reloading.
class HdPrman_ResourceRegistry final : public HdResourceRegistry 
{
public:
    HDPRMAN_API
    HdPrman_ResourceRegistry(
        std::shared_ptr<class HdPrman_RenderParam> const& renderParam);

    HDPRMAN_API
    ~HdPrman_ResourceRegistry() override;

    HDPRMAN_API
    void ReloadResource(
        TfToken const& resourceType,
        std::string const& path) override;

private:
    std::shared_ptr<class HdPrman_RenderParam> _renderParam;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RESOURCE_REGISTRY_H
