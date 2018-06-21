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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

// Static member initialization
const NdrTokenVec& NdrParserPlugin::DiscoveryTypes =
    {TfToken("unknown discovery type")};
const TfToken& NdrParserPlugin::SourceType =
    TfToken("unknown source type");

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
            NdrParserPlugin::DiscoveryTypes.front(),
            NdrParserPlugin::SourceType,
            dr.uri,
            /* properties = */ NdrPropertyUniquePtrVec(),
            /* metadata = */ NdrTokenMap()
        )
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
