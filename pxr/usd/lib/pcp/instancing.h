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
#ifndef PCP_INSTANCING_H
#define PCP_INSTANCING_H

/// \file pcp/instancing.h
///
/// A collection of private helper utilities to support instancing
/// functionality.

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/primIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Helper function to determine whether the given prim index is
/// instanceable. An instanceable prim index must have instanceable
/// nodes and must have been tagged so that the composed value of
/// the metadata field 'instance' is true.
bool
Pcp_PrimIndexIsInstanceable(
    const PcpPrimIndex& primIndex);

/// Helper function for traversing a prim index in strong-to-weak
/// order while identifying instanceable nodes. This function passes
/// each node in \p primIndex to the supplied \p visitor along with
/// a flag indicating whether that node is instanceable.
///
/// The visitor needs to implement this interface:
///
/// struct Visitor {
///     bool Visit(PcpNodeRef node, bool nodeIsInstanceable)
///     { ... }
/// }
///
/// If the Visit function returns false, traversal will be pruned at that
/// node and none of the node's children will be visited.
///
template <class Visitor>
void 
Pcp_TraverseInstanceableStrongToWeak(
    const PcpPrimIndex& primIndex,
    Visitor* visitor);

/// Helper function for traversing a prim index in weak-to-strong
/// order while identifying instanceable nodes. This function passes
/// each node in \p primIndex to the supplied \p visitor along with
/// a flag indicating whether that node is instanceable.
///
/// The visitor needs to implement this interface:
///
/// struct Visitor {
///     void Visit(PcpNodeRef node, bool nodeIsInstanceable)
///     { ... }
/// }
///
template <class Visitor>
void 
Pcp_TraverseInstanceableWeakToStrong(
    const PcpPrimIndex& primIndex, 
    Visitor* visitor);

// Implementation ----------------------------------------

inline bool
Pcp_ChildNodeIsInstanceable(
    const PcpNodeRef& node)
{
    // Non-ancestral nodes are instanceable: they represent a direct
    // composition arc to a portion of scenegraph that could be shared
    // with other prim indexes, as long as the other criteria laid out
    // in PcpInstanceKey are met. 
    return !node.IsDueToAncestor();
}

template <class Visitor>
inline void
Pcp_TraverseInstanceableStrongToWeakHelper(
    const PcpNodeRef& node,
    Visitor* visitor)
{
    // If the node is culled, the entire subtree rooted at this node
    // does not contribute to the prim index, so we can prune the 
    // traversal.
    if (node.IsCulled()) {
        return;
    }

    if (!visitor->Visit(node, Pcp_ChildNodeIsInstanceable(node))) {
        return;
    }

    TF_FOR_ALL(childIt, Pcp_GetChildrenRange(node)) {
        const PcpNodeRef& childNode = *childIt;
        Pcp_TraverseInstanceableStrongToWeakHelper(childNode, visitor);
    }
}

template <class Visitor>
inline void 
Pcp_TraverseInstanceableStrongToWeak(
    const PcpPrimIndex& primIndex,
    Visitor* visitor)
{
    const PcpNodeRef& rootNode = primIndex.GetRootNode();
    if (!visitor->Visit(rootNode, /* nodeIsInstanceable = */ false)) {
        return;
    }

    TF_FOR_ALL(childIt, Pcp_GetChildrenRange(rootNode)) {
        const PcpNodeRef& childNode = *childIt;
        Pcp_TraverseInstanceableStrongToWeakHelper(childNode, visitor);
    }
}

template <class Visitor>
inline void
Pcp_TraverseInstanceableWeakToStrongHelper(
    const PcpNodeRef& node,
    Visitor* visitor)
{
    // If the node is culled, the entire subtree rooted at this node
    // does not contribute to the prim index, so we can prune the 
    // traversal.
    if (node.IsCulled()) {
        return;
    }

    TF_REVERSE_FOR_ALL(childIt, Pcp_GetChildrenRange(node)) {
        const PcpNodeRef& childNode = *childIt;
        Pcp_TraverseInstanceableWeakToStrongHelper(childNode, visitor);
    }

    visitor->Visit(node, Pcp_ChildNodeIsInstanceable(node));
}

template <class Visitor>
inline void 
Pcp_TraverseInstanceableWeakToStrong(
    const PcpPrimIndex& primIndex, 
    Visitor* visitor)
{
    const PcpNodeRef& rootNode = primIndex.GetRootNode();
    TF_REVERSE_FOR_ALL(childIt, Pcp_GetChildrenRange(rootNode)) {
        const PcpNodeRef& childNode = *childIt;
        Pcp_TraverseInstanceableWeakToStrongHelper(childNode, visitor);
    }

    visitor->Visit(rootNode, /* nodeIsInstanceable = */ false);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_INSTANCING_H
