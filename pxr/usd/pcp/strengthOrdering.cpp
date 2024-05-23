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
#include "pxr/usd/pcp/utils.h"

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

// Returns the namespace depth of the node that inherits or specializes
// the class hierarchy that n is a member of.
static int
_GetNamespaceDepthForClassHierarchy(const PcpNodeRef& n)
{
    PcpNodeRef instanceNode, classNode;
    std::tie(instanceNode, classNode) = Pcp_FindStartingNodeOfClassHierarchy(n);

    // For specializes strength ordering, a relocates node isn't really the 
    // "instance" of the class hierarchy, it's just a placeholder indicating
    // a change in namespace. Skip past these nodes.
    while (instanceNode.GetArcType() == PcpArcTypeRelocate) {
        instanceNode = instanceNode.GetParentNode();
    }

    return instanceNode.GetNamespaceDepth();
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
    // nodes throughout the graph are copied to the root.
    if (PcpIsSpecializeArc(a.GetArcType())) {
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
        const PcpNodeRef aOrigin = a.GetOriginNode();
        const PcpNodeRef bOrigin = b.GetOriginNode();

        const bool aIsAuthoredArc = (aOrigin == a.GetParentNode());
        const bool bIsAuthoredArc = (bOrigin == b.GetParentNode());

        if (aOrigin == bOrigin) {
            // If a and b have the same origin node, either both are authored
            // arcs or both are implied/propagated from other nodes.
            //
            // - In the first case, we want to fall through to comparing the
            //   sibling arc numbers below.
            // 
            // - The second case should only happen when one node has been
            //   implied across a composition arc to the root node and the other
            //   has been propagated to the root node for strength ordering.
            //   The implied node expresses the opinions local to the root
            //   layer stack, and is stronger than the propagated node. The
            //   implied node will be the one whose site is *not* the same as
            //   its origin; the propagated node is the one whose site is the
            //   same as its origin. For more info on the propagation process,
            //   see documentation on _EvalImpliedSpecializes.
            if (!aIsAuthoredArc && !bIsAuthoredArc) {
                TF_VERIFY(a.GetParentNode() == a.GetRootNode() &&
                          b.GetParentNode() == b.GetRootNode());

                const bool aIsImplied = (a.GetSite() != aOrigin.GetSite());
                const bool bIsImplied = (b.GetSite() != bOrigin.GetSite());
                if (aIsImplied && !bIsImplied) {
                    return -1;
                }
                else if (!aIsImplied && bIsImplied) {
                    return 1;
                }
            
                TF_VERIFY(false, "Did not find copied specialize node.");
                return 0;
            }

            TF_VERIFY(aIsAuthoredArc && bIsAuthoredArc);
        }
        else {
            // If a and b originate from different authored specialize
            // arcs, use the strength ordering of the corresponding origin
            // root nodes to determine whether a or b is stronger.
            if (aOriginRoot.first != bOriginRoot.first) {
                const int result = _OriginIsStronger(
                    a.GetRootNode(), aOriginRoot.first, bOriginRoot.first);
                TF_VERIFY(result != 0, "Did not find either origin root");
                return result;
            }

            // Otherwise, a and b are sibling nodes that originate from the same
            // authored specialize arc. This can only happen if these nodes are
            // children of the root node. a and b may have been propagated or
            // implied from elsewhere in the composition graph or they may be
            // specialize arcs authored directly on the prim.
            TF_VERIFY(a.GetParentNode() == a.GetRootNode() &&
                      b.GetParentNode() == b.GetRootNode());

            // First, look at the namespace depth of the 'instance' node that
            // inherits or specializes the class hierarchy that a and b's origin
            // is a member of. This accounts for specializes arcs that have
            // been implied to to ancestral hierarchies. This shows up in
            // the museum test case SpecializesAndAncestralArcs2.
            const int aDepth = aIsAuthoredArc ? 
                0 : _GetNamespaceDepthForClassHierarchy(aOrigin);
            const int bDepth = bIsAuthoredArc ?
                0 : _GetNamespaceDepthForClassHierarchy(bOrigin);

            if (aDepth < bDepth) {
                return -1;
            }
            else if (bDepth < aDepth) {
                return 1;
            }

            // Next, check which node has the longest chain of origins
            // to the origin root. A node with a longer chain of origins
            // has been implied up the composition graph further away
            // from the origin root and closer to the root of the graph.
            // Conceptually, this means that a node with a longer chain
            // of origins represents an implied opinion that is "more
            // local" and thus stronger than a node with a shorter
            // chain of origins.
            //
            // For example, consider a simple chain of references with
            // a specializes arc at the end:
            //
            // @root.sdf@</A> -ref-> @ref.sdf@</Ref_1> 
            //                -ref-> @ref2.sdf@</Ref_2>
            //                -ref-> @ref3.sdf@</Ref_3> -spec-> @ref3.sdf@</S>
            //
            // The implied opinions due to @ref3.sdf@</S> are:
            //
            // @ref2.usda@</S> (origin = @ref3.usda@</S>, 
            //                  distance from origin root = 1)
            // @ref.usda@</S>, (origin = @ref2.usda@</S>, 
            //                  distance from origin root = 2)
            // @root.usda@</S>, (origin = @ref.usda@</S>,
            //                   distance from origin root = 3)
            //
            // Note that the longer the origin root distance, the
            // closer to the root and more local the opinion is.
            if (aOriginRoot.second > bOriginRoot.second) {
                return -1;
            }
            else if (bOriginRoot.second > aOriginRoot.second) {
                return 1;
            }

            // In cases where an inherited prim specializes another prim,
            // a and b may be two nodes where one has been implied
            // from the origin root up to the root node, and the other
            // has been propagated from under the inherited hierarchy
            // that was implied to the root node. In this case, the
            // implied node is the more local opinion. Without this
            // special handling, the traversal in the next block of
            // code would identify the propagated node as stronger.
            // This is similar to the special case above. See 
            // the TrickySpecializesAndInherits3 test case for an example.
            if (a.GetLayerStack() == a.GetRootNode().GetLayerStack() &&
                b.GetLayerStack() == b.GetRootNode().GetLayerStack() &&
                !aIsAuthoredArc && !bIsAuthoredArc) {
                
                const bool aIsImplied = (a.GetSite() != aOrigin.GetSite());
                const bool bIsImplied = (b.GetSite() != bOrigin.GetSite());
                if (aIsImplied && !bIsImplied) {
                    return -1;
                }
                else if (!aIsImplied && bIsImplied) {
                    return 1;
                }
            }

            // Lastly, traverse the graph to determine whether a or b's
            // origin is stronger.
            const int result = _OriginIsStronger(
                a.GetRootNode(), aOrigin, bOrigin);
            TF_VERIFY(result != 0, "Did not find either origin");
            return result;
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

int
PcpCompareSiblingPayloadNodeStrength(const PcpNodeRef& payloadParent, 
    int payloadArcNum, const PcpNodeRef& siblingNode) {
    if (payloadParent != siblingNode.GetParentNode()) {
        TF_CODING_ERROR("Nodes are not siblings");
        return 0;
    }

    // ArcType.
    // We rely on the enum values being in strength order.
    if (PcpArcTypePayload < siblingNode.GetArcType())
        return -1;
    if (PcpArcTypePayload > siblingNode.GetArcType())
        return 1;

    // Origin namespace depth.
    // Higher values (deeper opinions) are stronger.
    if (payloadParent.GetNamespaceDepth() > siblingNode.GetNamespaceDepth())
        return -1;
    if (payloadParent.GetNamespaceDepth() < siblingNode.GetNamespaceDepth())
        return 1;

    // Origin sibling arc number.
    // Lower numbers are stronger.
    if (payloadArcNum < siblingNode.GetSiblingNumAtOrigin())
        return -1;
    if (payloadArcNum > siblingNode.GetSiblingNumAtOrigin())
        return 1;

    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
