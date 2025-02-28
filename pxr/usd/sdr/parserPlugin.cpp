//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register this plugin type with Tf
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdrParserPlugin,
                   TfType::Bases<NdrParserPlugin>>();
}

SdrParserPlugin::SdrParserPlugin()
{
    // nothing yet
}

SdrParserPlugin::~SdrParserPlugin()
{
    // nothing yet
}

NdrNodeUniquePtr
SdrParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult) {

    SdrShaderNodeDiscoveryResult shaderResult =
        SdrShaderNodeDiscoveryResult::FromNdrNodeDiscoveryResult(
            discoveryResult
        );

    SdrShaderNodeUniquePtr node = ParseShaderNode(shaderResult);
    return node;
}

SdrShaderNodeUniquePtr
SdrParserPlugin::GetInvalidShaderNode(const SdrShaderNodeDiscoveryResult& dr)
{
    // Although the discovery result's "discovery type" could be used as the
    // node's type, that would expose an internal type that is not intended to
    // be visible to the outside. Instead, just use the generic "unknown" type.
    return SdrShaderNodeUniquePtr(
        new SdrShaderNode(
            dr.identifier,
            dr.version,
            dr.name,
            dr.family,
            TfToken("unknown discovery type"),
            TfToken("unknown source type"),
            dr.resolvedUri,
            dr.resolvedUri,
            /* properties = */ SdrShaderPropertyUniquePtrVec()
        )
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
