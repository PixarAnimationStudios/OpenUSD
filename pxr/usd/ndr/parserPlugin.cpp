//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/ndr/parserPlugin.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register this plugin type with Tf
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<NdrParserPlugin>();
}

NdrParserPlugin::NdrParserPlugin()
{
    // nothing yet
}

NdrParserPlugin::~NdrParserPlugin()
{
    // nothing yet
}

NdrNodeUniquePtr
NdrParserPlugin::GetInvalidNode(const NdrNodeDiscoveryResult& dr)
{
    // Although the discovery result's "discovery type" could be used as the
    // node's type, that would expose an internal type that is not intended to
    // be visible to the outside. Instead, just use the generic "unknown" type.
    return NdrNodeUniquePtr(
        new NdrNode(
            dr.identifier,
            dr.version,
            dr.name,
            dr.family,
            TfToken("unknown discovery type"),
            TfToken("unknown source type"),
            dr.resolvedUri,
            dr.resolvedUri,
            /* properties = */ NdrPropertyUniquePtrVec()
        )
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
