//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_PLUGIN_USD_SHADERS_DISCOVERY_PLUGIN_H
#define PXR_USD_IMAGING_PLUGIN_USD_SHADERS_DISCOVERY_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadersDiscoveryPlugin : public SdrDiscoveryPlugin {
public:
    UsdShadersDiscoveryPlugin() = default;

    ~UsdShadersDiscoveryPlugin() override = default;
    
    virtual SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(const Context &context) 
        override;

    virtual const SdrStringVec& GetSearchURIs() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_USD_SHADERS_DISCOVERY_PLUGIN_H
