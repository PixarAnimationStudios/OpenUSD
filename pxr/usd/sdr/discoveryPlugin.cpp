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
    TfType::Define<SdrDiscoveryPlugin>();
}

SdrDiscoveryPlugin::SdrDiscoveryPlugin()
{
    // nothing yet
}

SdrDiscoveryPlugin::~SdrDiscoveryPlugin()
{
    // nothing yet
}

PXR_NAMESPACE_CLOSE_SCOPE
