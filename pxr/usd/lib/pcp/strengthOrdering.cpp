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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/strengthOrdering.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

// Walk the entire expression tree under node, looking for either a or b.
// This is a helper used for resolving implied inherit strength.
static int
_OriginIsStronger(const PcpNodeRef& node,
                  const PcpNodeRef& a,
                  const PcpNodeRef& b)
{
    if (node == a)
        return -1;
    if (node == b)
        return 1;

    TF_FOR_ALL(child, Pcp_GetChildrenRange(node)) {
        int result = _OriginIsStronger(*child, a, b);
        if (result != 0)
            return result;
    }
    return 0;
}

// Walk the chain of origins for the given node and return the start
// of that chain, along with the number of origin nodes encountered
// along the way. This is similar to PcpNodeRef::GetOriginRootNode.
static std::pair<PcpNodeRef, size_t>
_GetOriginRootNode(const PcpNodeRef& node)
{
    std::pair<PcpNodeRef, size_t> result(node, 0);

    while (result.first.GetOriginNode() != result.first.GetParentNode()) {
        result.first = result.first.GetOriginNode();
        ++result.second;
    }
    return result;
}

// Return true if node a is a descendent of node b or vice-versa.
static bool
_OriginsAreNestedArcs(const PcpNodeRef& a, const PcpNodeRef& b)
{
    for (PcpNodeRef n = a; n; n = n.GetParentNode()) {
        if (n == b) {
            return true;
        }
    }

    for (PcpNodeRef n = b; n; n = n.GetParentNode()) {
        if (n == a) {
            return true;
        }
    }

    return false;
}

int 
PcpCompareSiblingNodeStrength(
    const PcpNodeRef& a, const PcpNodeRef& b)
{
    if (a.GetParentNode() != b.GetParentNode()) {
        TF_CODING_ERROR("Nodes are not siblings");
        return 0;
    }

    if (a == b) {
        return 0;
    }

    // ArcType.
    // We rely on the enum values being in strength order.
    if (a.GetArcType() < b.GetArcType())
        return -1;
    if (a.GetArcType() > b.GetArcType())
        return 1;

    // Specializes arcs need special handling because of how specializes
    // nodes throughout the graph are propagated to the root.
    if (PcpIsSpecializesArc(a.GetArcType())) {
        const PcpNodeRef aOrigin = a.GetOriginNode();
        const PcpNodeRef bOrigin = b.GetOriginNode();

        // Special case: We should only have two implied specializes nodes
        // with the same origin and that are siblings when one has been 
        // implied across a composition arc to the root node and the other 
        // has been propagated (i.e., copied) to the root node. In this
        // case, the implied arc -- the one whose opinions come from the
        // root layer stack -- is more local, and thus stronger
        if (aOrigin == bOrigin           &&
            aOrigin != a.GetParentNode() &&
            bOrigin != b.GetParentNode()) {

            TF_VERIFY(a.GetParentNode() == a.GetRootNode() &&
                      b.GetParentNode() == b.GetRootNode());

            if (a.GetLayerStack() == a.GetRootNode().GetLayerStack()) {
                return -1;
            }
            else if (b.GetLayerStack() == b.GetRootNode().GetLayerStack()) {
                return 1;
            }
            
            TF_VERIFY(false, "Did not find node with root layer stack.");
            return 0;
        }

        const std::pair<PcpNodeRef, size_t> aOriginRoot = _GetOriginRootNode(a);
        const std::pair<PcpNodeRef, size_t> bOriginRoot = _GetOriginRootNode(b);

        // Origin namespace depth.
        // Higher values (deeper opinions) are stronger, in general. However,
        // if one of the origin roots is somewhere beneath the other in the
        // graph, there must be a specializes arc somewhere between the two.
        // Specializes means that opinions for the source of the arc must be
        // weaker than the target, regardless of the namespace depth.
        if (!_OriginsAreNestedArcs(aOriginRoot.first, bOriginRoot.first)) {
            if (a.GetNamespaceDepth() > b.GetNamespaceDepth())
                return -1;
            if (a.GetNamespaceDepth() < b.GetNamespaceDepth())
                return 1;
        }
        
        // Origin strength.
        // Since specializes arcs are the weakest of all arc types, 
        // using 'strongest origin wins' would cause opinions that are 
        // more remote (e.g., across references) to be stronger than 
        // opinions that are more local. 
        //
        // To avoid this, we use the origin root node -- the node for the
        // actual authored opinion -- to determine strength.
        if (aOrigin != bOrigin) {
            if (aOriginRoot.first == bOriginRoot.first) {
                // If both sibling nodes have the same origin root, the
                // node with the longest chain of origins represents the
                // most local opinion, which should be strongest.
                if (aOriginRoot.second > bOriginRoot.second) {
                    return -1;
                }
                else if (bOriginRoot.second > aOriginRoot.second) {
                    return 1;
                }

                TF_VERIFY(
                    aOriginRoot.second != bOriginRoot.second,
                    "Should not have sibling specializes nodes with same "
                    "origin root and distance to origin root.");
            }
            else {
                // Otherwise, stronger origin root is stronger.
                int result = _OriginIsStronger(
                    a.GetRootNode(), aOriginRoot.first, bOriginRoot.first);
                if (result < 0) {
                    return -1;
                } else if (result > 0) {
                    return 1;
                }
                TF_VERIFY(false, "Did not find either origin");
            }
        }
    }
    else {
        // Origin namespace depth.
        // Higher values (deeper opinions) are stronger.
        if (a.GetNamespaceDepth() > b.GetNamespaceDepth())
            return -1;
        if (a.GetNamespaceDepth() < b.GetNamespaceDepth())
            return 1;

        // Origin strength.
        // Stronger origin is stronger.
        const PcpNodeRef aOrigin = a.GetOriginNode();
        const PcpNodeRef bOrigin = b.GetOriginNode();

        if (aOrigin != bOrigin) {
            // Walk the entire expression tree in strength order
            // to find which of a or b's origin comes first.
            int result = _OriginIsStronger(a.GetRootNode(), aOrigin, bOrigin);
            if (result < 0) {
                return -1;
            } else if (result > 0) {
                return 1;
            }
            TF_VERIFY(false, "Did not find either origin");
        }
    }

    // Origin sibling arc number.
    // Lower numbers are stronger.
    if (a.GetSiblingNumAtOrigin() < b.GetSiblingNumAtOrigin())
        return -1;
    if (a.GetSiblingNumAtOrigin() > b.GetSiblingNumAtOrigin())
        return 1;

    return 0;
}

// Walk from the given node to the root, collecting all of the nodes
// encountered along the way.
static PcpNodeRefVector
_CollectNodesFromNodeToRoot(PcpNodeRef node)
{
    PcpNodeRefVector nodes;
    for (; node; node = node.GetParentNode()) {
        nodes.push_back(node);
    }

    return nodes;
}

static int
_CompareNodeStrength(
    const PcpNodeRef& a, const PcpNodeRefVector& aNodes, 
    const PcpNodeRef& b, const PcpNodeRefVector& bNodes)
{
    // std::mismatch iterates through every nodes in aNodes. So, ensure that
    // there are enough corresponding elements in bNodes, flipping the
    // arguments and return value if necessary.
    if (bNodes.size() < aNodes.size()) {
        return -_CompareNodeStrength(b, bNodes, a, aNodes);
    }

    TF_VERIFY(aNodes.size() <= bNodes.size());

    // Search the two paths through the prim index graph to find the
    // lowest common parent node and the two siblings beneath that parent.
    std::pair<PcpNodeRefVector::const_reverse_iterator, 
              PcpNodeRefVector::const_reverse_iterator> nodesUnderCommonParent = 
        std::mismatch(aNodes.rbegin(), aNodes.rend(), bNodes.rbegin());
    
    // If the two paths through the graph diverge at some point, we should
    // have found a mismatch above. If we didn't, it must mean that the two
    // paths did not diverge, i.e., aNodes must be a subset of bNodes.
    // In that case, node \p a must be above node \p b in the graph, so it
    // must be stronger.
    if (nodesUnderCommonParent.first == aNodes.rend()) {
#ifdef PCP_DIAGNOSTIC_VALIDATION
        TF_VERIFY(std::find(bNodes.begin(), bNodes.end(), a) != bNodes.end());
#endif
        TF_VERIFY(nodesUnderCommonParent.second != bNodes.rend());
        return -1;
    }

    // Otherwise, compare the two sibling nodes to see which is stronger.
    TF_VERIFY(nodesUnderCommonParent.first != aNodes.rend() &&
              nodesUnderCommonParent.second != bNodes.rend());
    return PcpCompareSiblingNodeStrength(
        *nodesUnderCommonParent.first, *nodesUnderCommonParent.second);
}

int
PcpCompareNodeStrength(
    const PcpNodeRef& a, const PcpNodeRef& b)
{
    if (a.GetRootNode() != b.GetRootNode()) {
        TF_CODING_ERROR("Nodes are not part of the same prim index");
        return 0;
    }

    if (a == b) {
        return 0;
    }

    const PcpNodeRefVector aNodes = _CollectNodesFromNodeToRoot(a);
    const PcpNodeRefVector bNodes = _CollectNodesFromNodeToRoot(b);
    return _CompareNodeStrength(a, aNodes, b, bNodes);
}

PXR_NAMESPACE_CLOSE_SCOPE
