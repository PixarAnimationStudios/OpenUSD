//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/usd/ndr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    static TfToken _sourceType = TfToken("OSL");
    static NdrTokenVec _discoveryTypes = {TfToken("oso")};
}

class _NdrOslTestParserPlugin : public NdrParserPlugin
{
public:
    _NdrOslTestParserPlugin() {};
    ~_NdrOslTestParserPlugin() {};

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
                discoveryResult.uri,
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

const NdrTokenVec& _NdrOslTestParserPlugin::DiscoveryTypes = _discoveryTypes;
const TfToken& _NdrOslTestParserPlugin::SourceType = _sourceType;

NDR_REGISTER_PARSER_PLUGIN(_NdrOslTestParserPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
