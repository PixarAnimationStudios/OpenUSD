//
// Copyright 2016 Pixar
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
/// \file Dependencies.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependencies.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stl.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

Pcp_Dependencies::Pcp_Dependencies()
{
    // Do nothing
}

Pcp_Dependencies::~Pcp_Dependencies()
{
    // Do nothing
}

// Determine if Pcp_Dependencies should store an entry
// for the arc represented by the given node.
//
// As a space optimization, Pcp_Dependencies does not store entries
// for arcs that are implied by nearby structure and which can
// be easily synthesized. Specifically, it does not store arcs
// introduced purely ancestrally, nor does it store arcs for root nodes
// (PcpDependencyTypeRoot).
inline static bool
_ShouldStoreDependency(PcpDependencyFlags depFlags)
{
    return depFlags & PcpDependencyTypeDirect;
}

void
Pcp_Dependencies::Add(const PcpPrimIndex &primIndex)
{
    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies::Add");
    if (!primIndex.GetRootNode()) {
        return;
    }
    const SdfPath& primIndexPath = primIndex.GetRootNode().GetPath();
    TF_DEBUG(PCP_DEPENDENCIES)
        .Msg("Pcp_Dependencies: Adding deps for index <%s>:\n",
             primIndexPath.GetText());

    int nodeIndex=0, count=0;
    for (const PcpNodeRef &n: primIndex.GetNodeRange()) {
        const int curNodeIndex = nodeIndex++;
        const PcpDependencyFlags depFlags = PcpClassifyNodeDependency(n);
        if (_ShouldStoreDependency(depFlags)) {
            _SiteDepMap &siteDepMap = _deps[n.GetLayerStack()];
            std::vector<SdfPath> &deps = siteDepMap[n.GetPath()];

            TF_DEBUG(PCP_DEPENDENCIES)
                .Msg(" - Node %i (%s %s): <%s> %s\n",
                     curNodeIndex,
                     PcpDependencyFlagsToString(depFlags).c_str(),
                     TfEnum::GetDisplayName(n.GetArcType()).c_str(),
                     n.GetPath().GetText(),
                     TfStringify(n.GetLayerStack()->GetIdentifier()).c_str());

            deps.push_back(primIndexPath);
            count++;
        }
    }
    if (count == 0) {
        TF_DEBUG(PCP_DEPENDENCIES).Msg("    None\n");
    }
}

void
Pcp_Dependencies::Remove(const PcpPrimIndex &primIndex, PcpLifeboat *lifeboat)
{
    if (!primIndex.GetRootNode()) {
        return;
    }
    const SdfPath& primIndexPath = primIndex.GetRootNode().GetPath();
    TF_DEBUG(PCP_DEPENDENCIES)
        .Msg("Pcp_Dependencies: Removing deps for index <%s>\n",
             primIndexPath.GetText());

    int nodeIndex=0;
    for (const PcpNodeRef &n: primIndex.GetNodeRange()) {
        const int curNodeIndex = nodeIndex++;
        const PcpDependencyFlags depFlags = PcpClassifyNodeDependency(n);
        if (!_ShouldStoreDependency(depFlags)) {
            continue;
        }

        _SiteDepMap &siteDepMap = _deps[n.GetLayerStack()];
        std::vector<SdfPath> &deps = siteDepMap[n.GetPath()];

        TF_DEBUG(PCP_DEPENDENCIES)
            .Msg(" - Node %i (%s %s): <%s> %s\n",
                 curNodeIndex,
                 PcpDependencyFlagsToString(depFlags).c_str(),
                 TfEnum::GetDisplayName(n.GetArcType()).c_str(),
                 n.GetPath().GetText(),
                 TfStringify(n.GetLayerStack()->GetIdentifier()).c_str());

        // Swap with last element, then remove that.
        // We are using the vector as an unordered set.
        std::vector<SdfPath>::iterator i =
            std::find(deps.begin(), deps.end(), primIndexPath);
        if (!TF_VERIFY(i != deps.end())) {
            continue;
        }
        std::vector<SdfPath>::iterator last = --deps.end();
        std::swap(*i, *last);
        deps.erase(last);

        // Reap container entries when no deps are left.
        // This is slightly tricky with SdfPathTable since we need
        // to examine subtrees and parents.
        if (deps.empty()) {
            TF_DEBUG(PCP_DEPENDENCIES).Msg("      Removed last dep on site\n");

            // Scan children to see if we can remove this subtree.
            _SiteDepMap::iterator i, iBegin, iEnd;
            std::tie(iBegin, iEnd) = siteDepMap.FindSubtreeRange(n.GetPath());
            for (i = iBegin; i != iEnd && i->second.empty(); ++i) {}
            bool subtreeIsEmpty = i == iEnd;
            if (subtreeIsEmpty) {
                siteDepMap.erase(iBegin);
                TF_DEBUG(PCP_DEPENDENCIES).Msg("      No subtree deps\n");

                // Now scan upwards to reap parent entries.
                for (SdfPath p = n.GetPath().GetParentPath();
                     !p.IsEmpty(); p = p.GetParentPath()) {
                    std::tie(iBegin, iEnd) = siteDepMap.FindSubtreeRange(p);
                    if (iBegin != iEnd
                        && std::next(iBegin) == iEnd
                        && iBegin->second.empty()) {
                    TF_DEBUG(PCP_DEPENDENCIES)
                        .Msg("    Removing empty parent entry <%s>\n",
                             p.GetText());
                        siteDepMap.erase(iBegin);
                    } else {
                        break;
                    }
                }

                // Check if the entire table is empty.
                if (siteDepMap.empty()) {
                    if (lifeboat) {
                        lifeboat->Retain(n.GetLayerStack());
                    }
                    _deps.erase(n.GetLayerStack());

                    TF_DEBUG(PCP_DEPENDENCIES)
                        .Msg("    Removed last dep on %s\n",
                             TfStringify(n.GetLayerStack()
                                         ->GetIdentifier()).c_str());
                }
            }
        }
    }
}

void
Pcp_Dependencies::RemoveAll(PcpLifeboat* lifeboat)
{
    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::RemoveAll: Clearing all dependencies\n");

    // Retain all layerstacks in the lifeboat.
    if (lifeboat) {
        TF_FOR_ALL(i, _deps) {
            lifeboat->Retain(i->first);
        }
    }

    _deps.clear();
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedLayers() const
{
    SdfLayerHandleSet reachedLayers;

    TF_FOR_ALL(layerStack, _deps) {
        const SdfLayerRefPtrVector& layers = layerStack->first->GetLayers();
        reachedLayers.insert(layers.begin(), layers.end());
    }

    return reachedLayers;
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedRootLayers() const
{
    SdfLayerHandleSet reachedRootLayers;

    TF_FOR_ALL(i, _deps) {
        const PcpLayerStackPtr& layerStack = i->first;
        reachedRootLayers.insert(layerStack->GetIdentifier().rootLayer );
    }

    return reachedRootLayers;
}

bool 
Pcp_Dependencies::UsesLayerStack(const PcpLayerStackPtr& layerStack) const
{
    return _deps.find(layerStack) != _deps.end();
}

PXR_NAMESPACE_CLOSE_SCOPE
