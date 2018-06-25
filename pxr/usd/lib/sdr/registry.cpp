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
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/usd/sdr/registry.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(SdrRegistry);

namespace {
    SdrShaderNodeConstPtr NdrNodeToShaderNode(NdrNodeConstPtr node)
    {
        return dynamic_cast<SdrShaderNodeConstPtr>(node);
    }

    SdrShaderNodePtrVec NdrNodeVecToShaderNodeVec(NdrNodeConstPtrVec nodeVec)
    {
        SdrShaderNodePtrVec sdrNodes;

        std::transform(nodeVec.begin(), nodeVec.end(),
            std::back_inserter(sdrNodes),
            [](NdrNodeConstPtr baseNode) {
                return SdrShaderNodeConstPtr(baseNode);
            }
        );

        return sdrNodes;
    }
}

SdrRegistry::SdrRegistry()
: NdrRegistry()
{
    // nothing yet
}

SdrRegistry::~SdrRegistry()
{
    // nothing yet
}

SdrRegistry&
SdrRegistry::GetInstance()
{
    return TfSingleton<SdrRegistry>::GetInstance();
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifier(
    const NdrIdentifier& identifier, const NdrTokenVec& typePriority)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeByIdentifier(identifier, typePriority)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifierAndType(
    const NdrIdentifier& identifier, const TfToken& nodeType)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeByIdentifierAndType(identifier, nodeType)
    );
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromAsset(
    const SdfAssetPath &shaderAsset,
    const NdrTokenMap &metadata)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeFromAsset(shaderAsset, metadata));
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromSourceCode(
    const std::string &sourceCode,
    const TfToken &sourceType,
    const NdrTokenMap &metadata)
{
    return NdrNodeToShaderNode(GetInstance().GetNodeFromSourceCode(
        sourceCode, sourceType, metadata));
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByName(
    const std::string& name, const NdrTokenVec& typePriority,
    NdrVersionFilter filter)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeByName(name, typePriority, filter)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByNameAndType(
    const std::string& name, const TfToken& nodeType,
    NdrVersionFilter filter)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeByNameAndType(name, nodeType, filter)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByURI(const std::string& uri)
{
    return NdrNodeToShaderNode(
        GetInstance().GetNodeByURI(uri)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByIdentifier(const NdrIdentifier& identifier)
{
    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByIdentifier(identifier)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByName(
    const std::string& name,
    NdrVersionFilter filter)
{
    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByName(name, filter)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByFamily(
    const TfToken& family,
    NdrVersionFilter filter)
{
    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByFamily(family, filter)
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
