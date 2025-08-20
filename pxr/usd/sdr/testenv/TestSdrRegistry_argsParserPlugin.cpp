//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    static TfToken _sourceType = TfToken("RmanCpp");
    static SdrTokenVec _discoveryTypes = {TfToken("args")};
}

class _SdrArgsTestParserPlugin : public SdrParserPlugin
{
public:
    _SdrArgsTestParserPlugin() {};
    ~_SdrArgsTestParserPlugin() {};

    SdrShaderNodeUniquePtr ParseShaderNode(
        const SdrShaderNodeDiscoveryResult& discoveryResult) override
    {
        return SdrShaderNodeUniquePtr(
            new SdrShaderNode(
                discoveryResult.identifier,
                discoveryResult.version,
                discoveryResult.name,
                discoveryResult.family,
                discoveryResult.sourceType,
                discoveryResult.sourceType,
                discoveryResult.resolvedUri,
                discoveryResult.resolvedUri,
                SdrShaderPropertyUniquePtrVec()
            )
        );
    }

    static const SdrTokenVec& DiscoveryTypes;
    static const TfToken& SourceType;

    const SdrTokenVec& GetDiscoveryTypes() const override {
        return _discoveryTypes;
    }

    const TfToken& GetSourceType() const override {
        return _sourceType;
    }
};

const SdrTokenVec& _SdrArgsTestParserPlugin::DiscoveryTypes = _discoveryTypes;
const TfToken& _SdrArgsTestParserPlugin::SourceType = _sourceType;

SDR_REGISTER_PARSER_PLUGIN(_SdrArgsTestParserPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
