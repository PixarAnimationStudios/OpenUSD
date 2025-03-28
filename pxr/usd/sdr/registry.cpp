//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
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
    // Track plugin discovery cost of base class
    TRACE_FUNCTION();
}

SdrRegistry::~SdrRegistry()
{
}

SdrRegistry&
SdrRegistry::GetInstance()
{
    return TfSingleton<SdrRegistry>::GetInstance();
}

void
SdrRegistry::AddDiscoveryResult(SdrShaderNodeDiscoveryResult&& discoveryResult)
{
    NdrNodeDiscoveryResult ndrResult =
        discoveryResult.ToNdrNodeDiscoveryResult();
    NdrRegistry::AddDiscoveryResult(std::move(ndrResult));
}

void
SdrRegistry::AddDiscoveryResult(
    const SdrShaderNodeDiscoveryResult& discoveryResult)
{
    // Explicitly create a copy, otherwise this method will recurse
    // into itself.
    SdrShaderNodeDiscoveryResult result = discoveryResult;
    AddDiscoveryResult(std::move(result));
}

SdrIdentifierVec
SdrRegistry::GetShaderNodeIdentifiers(
    const TfToken& family,
    SdrVersionFilter filter) const
{
    return NdrRegistry::GetNodeIdentifiers(
        family,
        SdrVersionFilterToNdr(filter));
}

SdrStringVec
SdrRegistry::GetShaderNodeNames(const TfToken& family) const
{
    return NdrRegistry::GetNodeNames(family);
}


SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifier(
    const NdrIdentifier& identifier, const NdrTokenVec& typePriority)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByIdentifier(identifier, typePriority)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifierAndType(
    const NdrIdentifier& identifier, const TfToken& nodeType)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByIdentifierAndType(identifier, nodeType)
    );
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromAsset(
    const SdfAssetPath &shaderAsset,
    const NdrTokenMap &metadata,
    const TfToken &subIdentifier,
    const TfToken &sourceType)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeFromAsset(shaderAsset,
                                       metadata,
                                       subIdentifier,
                                       sourceType));
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromSourceCode(
    const std::string &sourceCode,
    const TfToken &sourceType,
    const NdrTokenMap &metadata)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(GetInstance().GetNodeFromSourceCode(
        sourceCode, sourceType, metadata));
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByName(
    const std::string& name, const NdrTokenVec& typePriority,
    SdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByName(name, typePriority,
                                    SdrVersionFilterToNdr(filter))
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByName(
    const std::string& name, const NdrTokenVec& typePriority,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByName(name, typePriority, filter)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByNameAndType(
    const std::string& name, const TfToken& nodeType,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByNameAndType(name, nodeType, filter)
    );
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByNameAndType(
    const std::string& name, const TfToken& nodeType,
    SdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToShaderNode(
        GetInstance().GetNodeByNameAndType(name, nodeType,
                                           SdrVersionFilterToNdr(filter))
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByIdentifier(const NdrIdentifier& identifier)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByIdentifier(identifier)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByName(
    const std::string& name,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByName(name, filter)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByName(
    const std::string& name,
    SdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByName(name, SdrVersionFilterToNdr(filter))
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByFamily(
    const TfToken& family,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByFamily(family, filter)
    );
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByFamily(
    const TfToken& family,
    SdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToShaderNodeVec(
        GetInstance().GetNodesByFamily(family, SdrVersionFilterToNdr(filter))
    );
}

SdrTokenVec
SdrRegistry::GetAllShaderNodeSourceTypes() const
{
    return NdrRegistry::GetAllNodeSourceTypes();
}

PXR_NAMESPACE_CLOSE_SCOPE
