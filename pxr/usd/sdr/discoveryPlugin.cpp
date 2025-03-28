//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdr/discoveryPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register this plugin type with Tf
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdrDiscoveryPlugin,
                   TfType::Bases<NdrDiscoveryPlugin>>();
}

SdrDiscoveryPlugin::SdrDiscoveryPlugin()
{
    // nothing yet
}

SdrDiscoveryPlugin::~SdrDiscoveryPlugin()
{
    // nothing yet
}

/// _SdrDiscoveryPluginContextWrapper forwards calls to the
/// NdrDiscoveryPluginContext API in order to preserve existing behavior.
class _SdrDiscoveryPluginContextWrapper : public SdrDiscoveryPluginContext {
public:
    _SdrDiscoveryPluginContextWrapper(const NdrDiscoveryPluginContext& c)
        : _ndrContext(c) {}
    ~_SdrDiscoveryPluginContextWrapper() override = default;

    TfToken GetSourceType(const TfToken& discoveryType) const override
    {
        return _ndrContext.GetSourceType(discoveryType);
    }

private:
    const NdrDiscoveryPluginContext& _ndrContext;
};

NdrNodeDiscoveryResultVec
SdrDiscoveryPlugin::DiscoverNodes(const NdrDiscoveryPluginContext& c) {
    NdrNodeDiscoveryResultVec nodes;
    _SdrDiscoveryPluginContextWrapper wrapper(c);
    for (const SdrShaderNodeDiscoveryResult& result : DiscoverShaderNodes(wrapper)) {
        NdrNodeDiscoveryResult ndrRes = result.ToNdrNodeDiscoveryResult();
        nodes.push_back(ndrRes);
    }
    return nodes;
}

PXR_NAMESPACE_CLOSE_SCOPE
