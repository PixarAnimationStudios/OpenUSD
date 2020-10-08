//
// Copyright 2019 Pixar
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
#include "hdPrman/matfiltResolveVstructs.h"
#include "hdPrman/debugCodes.h"

#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (vstructmemberaliases)         \
    (enableVstructConditions)
);

namespace {

/// For a single vstruct placeholder input/output, this describes mappings from:
/// 1) associated member input/output -> member name (or names via alias)
/// 2) member name -> associated member input/output
/// 3) associated member input/output -> parsed conditional expression
struct _VstructInfoEntry {
    typedef std::shared_ptr<_VstructInfoEntry> Ptr;

    typedef std::unordered_map<TfToken, std::vector<TfToken>,
            TfToken::HashFunctor> MemberMap;
    typedef std::unordered_map<TfToken, TfToken,
            TfToken::HashFunctor> ReverseMemberMap;
    typedef std::unordered_map<TfToken, MatfiltVstructConditionalEvaluator::Ptr,
            TfToken::HashFunctor> ConditionalMap;

    MemberMap members;
    ReverseMemberMap reverseMembers;
    ConditionalMap conditionals;
};

/// For a single shader, this stores mappings from:
/// 1) vstruct placeholder input/output -> _VstructInfoEntry
///
/// These are typically built once per shader type and cached as it requires
/// interpretation of metadata which would be wasteful to do repeatedly.
struct _ShaderInfoEntry {    
    typedef std::shared_ptr<_ShaderInfoEntry> Ptr;
    typedef std::map<TfToken, _VstructInfoEntry::Ptr> VstructInfoMap;

    VstructInfoMap vstructs;

    /// Constructs a _ShaderInfoEntry::Ptr for a single shader without caching
    static Ptr
    Build(const TfToken & nodeTypeId,
          const NdrTokenVec & shaderTypePriority)
    {
        auto result = Ptr(new _ShaderInfoEntry);
        if (auto sdrShader =
                SdrRegistry::GetInstance().GetShaderNodeByIdentifier(
                        nodeTypeId, shaderTypePriority))
        {
            for (const auto & inputName : sdrShader->GetInputNames()) {
                auto sdrInput = sdrShader->GetShaderInput(inputName);
                if (!sdrInput) {
                    continue;
                }
                _ProcessProperty(result, sdrInput);
            }
            for (const auto & outputName : sdrShader->GetOutputNames()) {
                auto sdrOutput = sdrShader->GetShaderOutput(outputName);
                if (!sdrOutput) {
                    continue;
                }
                _ProcessProperty(result, sdrOutput);
            }

        }

        return result;

    }

    /// Constructs and caches a _ShaderInfoEntry::Ptr for a single shader
    static Ptr
    Get(const TfToken & nodeTypeId,
        const NdrTokenVec & shaderTypePriority)
    {
        std::lock_guard<std::mutex> lock(_cachedEntryMutex);

        auto I = _cachedEntries.find(nodeTypeId);
        if (I == _cachedEntries.end()) {
            auto result = Build(nodeTypeId, shaderTypePriority);
            _cachedEntries[nodeTypeId] = result;
            return result;
        }
        return (*I).second;
    }


private:
    static std::unordered_map<TfToken, Ptr, TfToken::HashFunctor>
            _cachedEntries;
    static std::mutex _cachedEntryMutex;

    static void
    _ProcessProperty(Ptr result, SdrShaderPropertyConstPtr prop)
    {
        if (!prop->IsVStructMember()) {
            return;
        }

        const TfToken & vsName = prop->GetVStructMemberOf();
        const TfToken & vsMemberName = prop->GetVStructMemberName();
        if (vsName.IsEmpty() || vsMemberName.IsEmpty()) {
            return;
        }

        _VstructInfoEntry::Ptr entry;
        auto I = result->vstructs.find(vsName);
        if (I != result->vstructs.end()) {
            entry = (*I).second;
        } else {
            entry.reset(new _VstructInfoEntry);
            result->vstructs[vsName] = entry;
        }

        TfToken vsMemberAlias;

        const auto & metadata = prop->GetMetadata();
        {
            auto I = metadata.find(_tokens->vstructmemberaliases);
            if (I != metadata.end()) {
                vsMemberAlias = TfToken((*I).second);
            }
        }

        auto & memberNames = entry->members[prop->GetName()];
        memberNames.push_back(vsMemberName);
        
        entry->reverseMembers[vsMemberName] = prop->GetName();

        if (!vsMemberAlias.IsEmpty()) {
            memberNames.push_back(vsMemberAlias);
            entry->reverseMembers[vsMemberAlias] = prop->GetName();
        }

        const TfToken & condExpr = prop->GetVStructConditionalExpr();

        if (!condExpr.IsEmpty()) {
            entry->conditionals[prop->GetName()] =
                    MatfiltVstructConditionalEvaluator::Parse(condExpr.data());
        }
    }
};

std::unordered_map<TfToken, _ShaderInfoEntry::Ptr, TfToken::HashFunctor>
        _ShaderInfoEntry::_cachedEntries;

std::mutex _ShaderInfoEntry::_cachedEntryMutex;

} // anonymous namespace

static void
_ResolveVstructsForNode(
    HdMaterialNetwork2 & network,
    const SdfPath & nodeId,
    HdMaterialNode2 & node,
    std::set<SdfPath> resolvedNodeNames,
    const NdrTokenVec & shaderTypePriority,
    bool enableConditions)
{
    if (resolvedNodeNames.find(nodeId) != resolvedNodeNames.end()) {
        TF_DEBUG(HDPRMAN_VSTRUCTS)
            .Msg("No resovled node name for %s\n", nodeId.GetText());
        return;
    }
    
    resolvedNodeNames.insert(nodeId);
    auto shaderInfo =
        _ShaderInfoEntry::Get(node.nodeTypeId, shaderTypePriority);

    // don't do anything if the node has no vstruct definitions
    if (shaderInfo->vstructs.empty()) {
        TF_DEBUG(HDPRMAN_VSTRUCTS)
            .Msg("Node %s has no vstructs\n", node.nodeTypeId.GetText());
        return;
    }

    // copy inputConnections as we may be modifying them mid-loop
    auto inputConnectionsCopy = node.inputConnections;

    for (auto & connectionI : inputConnectionsCopy) {

        const auto & inputName = connectionI.first;

        auto I = shaderInfo->vstructs.find(inputName);
        if (I == shaderInfo->vstructs.end()) {
            continue;
        }
        TF_DEBUG(HDPRMAN_VSTRUCTS)
            .Msg("Found input %s with a vstruct\n", inputName.GetText());

        const auto & vstructInfo = (*I).second;
        const auto & upstreamConnections = connectionI.second;

        if (upstreamConnections.empty()) {
            TF_DEBUG(HDPRMAN_VSTRUCTS)
                .Msg("Ignoring since no connection\n");
            continue;
        }
        TF_DEBUG(HDPRMAN_VSTRUCTS)
            .Msg("Found upstream vstruct connection\n");

        const auto & upstreamConnection = upstreamConnections.front();

        // confirm connected node exists
        auto upstreamNodeI =
                network.nodes.find(upstreamConnection.upstreamNode);
        if (upstreamNodeI == network.nodes.end()) {
            continue;
        }
        auto & upstreamNode = (*upstreamNodeI).second;

        // confirm connected upstream output is a vstruct
        auto upstreamShaderInfo = _ShaderInfoEntry::Get(
                upstreamNode.nodeTypeId, shaderTypePriority);
        auto upstreamVstructI = upstreamShaderInfo->vstructs.find(
                    upstreamConnection.upstreamOutputName);
        if (upstreamVstructI == upstreamShaderInfo->vstructs.end()) {
            continue;
        }

        auto upstreamVstruct = (*upstreamVstructI).second;

        // ensure that all connections/conditions are expanded upstream first
        _ResolveVstructsForNode(network, upstreamConnection.upstreamNode,
                                upstreamNode, resolvedNodeNames,
                                shaderTypePriority, enableConditions);

        // delete the placeholder connection
        node.inputConnections.erase(inputName);

        for (auto & memberI : vstructInfo->members) {
            const TfToken & memberInputName = memberI.first;

            // If there's an existing connection to a member input, skip
            // expansion as a direct connection has a stronger opinion.
            if (node.inputConnections.find(memberInputName)
                    != node.inputConnections.end()) {
                continue;
            }

            // loop over member names (which may be > 1 due to member aliases)
            for (const TfToken & memberName : memberI.second) {
                auto revMemberI =
                    upstreamVstruct->reverseMembers.find(memberName);

                if (revMemberI == upstreamVstruct->reverseMembers.end()) {
                    continue;
                }
                const TfToken & upstreamMemberOutputName = (*revMemberI).second;
                
                // check for condition, otherwise connect
                auto condI = upstreamVstruct->conditionals.find(
                        upstreamMemberOutputName);
                
                if (enableConditions &&
                        condI != upstreamVstruct->conditionals.end())
                {
                    auto & evaluator = (*condI).second;
                    evaluator->Evaluate(
                            nodeId,
                            memberInputName,
                            upstreamConnection.upstreamNode,
                            upstreamMemberOutputName,
                            shaderTypePriority,
                            network);
                } else {
                    // no condition, just connect
                    node.inputConnections[memberInputName] = {{
                            upstreamConnection.upstreamNode,
                            upstreamMemberOutputName}};
                    TF_DEBUG(HDPRMAN_VSTRUCTS)
                        .Msg("Connected condition-less %s.%s to %s.%s\n",
                            nodeId.GetText(),
                            memberInputName.GetText(),
                            upstreamConnection.upstreamNode.GetText(),
                            upstreamMemberOutputName.GetText());
                }  
                break;
            }
        }
    }
} 


void
MatfiltResolveVstructs(
    const SdfPath & networkId,
    HdMaterialNetwork2 & network,
    const std::map<TfToken, VtValue> & contextValues,
    const NdrTokenVec & shaderTypePriority,
    std::vector<std::string> * outputErrorMessages)
{
    std::set<SdfPath> resolvedNodeNames;

    bool enableConditions = true;

    auto I = contextValues.find(_tokens->enableVstructConditions);
    if (I != contextValues.end()) {
        const VtValue & value = (*I).second;
        if (value.IsHolding<bool>()) {
            enableConditions = value.UncheckedGet<bool>();
        }
    }

    for (auto & I : network.nodes) {
        const SdfPath & nodeId = I.first;
        HdMaterialNode2 & node = I.second;
        _ResolveVstructsForNode(
                network,
                nodeId,
                node,
                resolvedNodeNames,
                shaderTypePriority,
                enableConditions);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
