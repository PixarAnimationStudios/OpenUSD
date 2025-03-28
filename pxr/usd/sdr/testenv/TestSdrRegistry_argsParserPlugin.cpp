//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ndr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    static TfToken _sourceType = TfToken("RmanCpp");
    static NdrTokenVec _discoveryTypes = {TfToken("args")};
}

class _NdrArgsTestParserPlugin : public NdrParserPlugin
{
public:
    _NdrArgsTestParserPlugin() {};
    ~_NdrArgsTestParserPlugin() {};

    NdrNodeUniquePtr Parse(
        const NdrNodeDiscoveryResult& discoveryResult) override
    {
        return NdrNodeUniquePtr(
            new SdrShaderNode(
                discoveryResult.identifier,
                discoveryResult.version,
                discoveryResult.name,
                discoveryResult.family,
                discoveryResult.sourceType,
                discoveryResult.sourceType,
                discoveryResult.resolvedUri,
                discoveryResult.resolvedUri,
                NdrPropertyUniquePtrVec()
            )
        );
    }

    static const NdrTokenVec& DiscoveryTypes;
    static const TfToken& SourceType;

    const NdrTokenVec& GetDiscoveryTypes() const override {
        return _discoveryTypes;
    }

    const TfToken& GetSourceType() const override {
        return _sourceType;
    }
};

const NdrTokenVec& _NdrArgsTestParserPlugin::DiscoveryTypes = _discoveryTypes;
const TfToken& _NdrArgsTestParserPlugin::SourceType = _sourceType;

NDR_REGISTER_PARSER_PLUGIN(_NdrArgsTestParserPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
