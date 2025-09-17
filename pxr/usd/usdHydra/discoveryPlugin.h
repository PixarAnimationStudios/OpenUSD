//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_HYDRA_DISCOVERY_PLUGIN_H
#define PXR_USD_USD_HYDRA_DISCOVERY_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/usd/usdHydra/api.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdHydraDiscoveryPlugin : public SdrDiscoveryPlugin {
public:
    UsdHydraDiscoveryPlugin() = default;

    ~UsdHydraDiscoveryPlugin() override = default;
    
    virtual SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(const Context &context) 
        override;

    virtual const SdrStringVec& GetSearchURIs() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_HYDRA_DISCOVERY_PLUGIN_H
