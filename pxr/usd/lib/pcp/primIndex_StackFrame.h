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
#ifndef PCP_PRIM_INDEX_STACK_FRAME_H
#define PCP_PRIM_INDEX_STACK_FRAME_H

#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/site.h"

/// \class PcpPrimIndex_StackFrame
///
/// Internal helper class for tracking recursive invocations of
/// the prim indexing algorithm.
///
class PcpPrimIndex_StackFrame
{
public:
    PcpPrimIndex_StackFrame()
        : previousFrame(NULL)
        , skipDuplicateNodes(false)
        , arcToParent(NULL)
    { }

    /// Link to the previous recursive invocation.
    PcpPrimIndex_StackFrame* previousFrame;

    /// The site of the prim index being built by this recursive
    /// call to Pcp_BuildPrimIndex.
    PcpLayerStackSite requestedSite;

    /// Whether the prim index being built by this recursive call should
    /// skip adding nodes if another node exists with the same site.
    bool skipDuplicateNodes;

    /// The node in the parent graph that will be the parent of the prim index 
    /// being built by this recursive call.
    PcpNodeRef parentNode;

    /// The arc connecting the prim index being built by this recursive
    /// call to the parent node in the previous stack frame.
    PcpArc* arcToParent;
};

/// \class PcpPrimIndex_StackFrameIterator
///
/// Iterator for walking up a node's ancestors while potentially crossing
/// stack frames.
///
class PcpPrimIndex_StackFrameIterator
{
public:
    PcpNodeRef node;
    PcpPrimIndex_StackFrame* previousFrame;

    PcpPrimIndex_StackFrameIterator(
        const PcpNodeRef& n, PcpPrimIndex_StackFrame* f)
        : node(n)
        , previousFrame(f)
    {
    }

    /// Step to the next parent node.
    void Next() 
    {
        if (node.GetArcType() != PcpArcTypeRoot) {
            // Step to the next parent within this graph.
            node = node.GetParentNode();
        } else if (previousFrame) {
            // No more parents in this graph, but there is an outer
            // prim index that this node will become part of.
            // Step to the (eventual) parent in that graph.
            node = previousFrame->parentNode;
            previousFrame = previousFrame->previousFrame;
        } else {
            // No more parents.
            node = PcpNodeRef();
        }
    }

    /// Step to the first parent node in the next recursive call.
    void NextFrame() 
    {
        if (previousFrame) {
            node = previousFrame->parentNode;
            previousFrame = previousFrame->previousFrame;
        }
        else {
            node = PcpNodeRef();
        }
    }

    /// Get the type of arc connecting the current node with its parent.
    PcpArcType GetArcType() 
    {
        if (node.GetArcType() != PcpArcTypeRoot) {
            // Use the current node's arc type.
            return node.GetArcType();
        } else if (previousFrame) {
            // No more parents in this graph, but there is an outer
            // prim index, so consult arcToParent.
            return previousFrame->arcToParent->type;
        } else {
            // No more parents; this must be the absolute final root.
            return PcpArcTypeRoot;
        }
    }
};

#endif // PCP_PRIM_INDEX_STACK_FRAME_H
