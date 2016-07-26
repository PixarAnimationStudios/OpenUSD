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
#include "pxr/usd/pcp/payloadContext.h"

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/primIndex_StackFrame.h"
#include "pxr/usd/sdf/layer.h"

PcpPayloadContext::PcpPayloadContext(
    const PcpNodeRef& parentNode, 
    PcpPrimIndex_StackFrame* previousStackFrame)
    : _parentNode(parentNode)
    , _previousStackFrame(previousStackFrame)
{
}

namespace
{
enum _ComposeStatus
{
    _ComposeContinue,
    _ComposeStop
};

static _ComposeStatus
_ComposeStrongestOpinionAtNode(
    const PcpNodeRef& node,
    const TfToken& fieldName,
    const PcpPayloadContext::ComposeFunction& composeFn,
    bool* foundValue)
{
    const SdfAbstractDataSpecId specId(&node.GetPath());
    TF_FOR_ALL(layer, node.GetLayerStack()->GetLayers()) {
        VtValue value;
        if ((*layer)->HasField(specId, fieldName, &value)) {
            *foundValue = true;
            const _ComposeStatus status = 
                composeFn(&value) ? _ComposeStop : _ComposeContinue;
            if (status == _ComposeStop) {
                return status;
            }
        }
    }
    return _ComposeContinue;
}

static bool
_ComposeStrongestOpinionInSubtree(
    const PcpNodeRef& node,
    const TfToken& fieldName,
    const PcpPayloadContext::ComposeFunction& composeFn,
    bool* foundValue)
{
    if (_ComposeStrongestOpinionAtNode(
            node, fieldName, composeFn, foundValue) == _ComposeStop) {
        return _ComposeStop;
    }

    TF_FOR_ALL(childNode, Pcp_GetChildrenRange(node)) {
        if (_ComposeStrongestOpinionInSubtree(
                *childNode, fieldName, composeFn, foundValue) == _ComposeStop) {
            return _ComposeStop;
        }
    }

    return _ComposeContinue;
}

static bool
_ComposeStrongestOpinion(
    PcpPrimIndex_StackFrameIterator* iterator,
    const TfToken& fieldName,
    const PcpPayloadContext::ComposeFunction& composeFn,
    bool* foundValue)
{
    PcpNodeRef currentNode = iterator->node;

    // Try parent node.
    iterator->Next();
    if (iterator->node) {
        if (_ComposeStrongestOpinion(
                iterator, fieldName, composeFn, foundValue) == _ComposeStop) {
            return _ComposeStop;
        }
    }
    
    // Compose at the current node.
    if (_ComposeStrongestOpinionAtNode(
            currentNode, fieldName, composeFn, foundValue) == _ComposeStop) {
        return _ComposeStop;
    }

    TF_FOR_ALL(childNode, Pcp_GetChildrenRange(currentNode)) {
        if (_ComposeStrongestOpinionInSubtree(
                *childNode, fieldName, composeFn, foundValue)) {
            return true;
        }
    }

    return false;
}
} // end anonymous namspace

bool 
PcpPayloadContext::ComposeValue(
    const TfToken& fieldName, const ComposeFunction& fn) const
{
    PcpPrimIndex_StackFrameIterator 
        iterator(_parentNode, _previousStackFrame);

    // This function will be invoked prior to the addition of a new
    // payload arc. Since we know this new payload node will always
    // be the weakest child of _parentNode (see strengthOrdering.cpp)
    // we simply compose the strongest opinion that exists in the
    // current prim index.
    bool foundValue = false;
    _ComposeStrongestOpinion(&iterator, fieldName, fn, &foundValue);
    return foundValue;
}

// "Private" function for creating a PcpPayloadContext; should only
// be used by prim indexing.
PcpPayloadContext 
Pcp_CreatePayloadContext(
    const PcpNodeRef& parentNode, 
    PcpPrimIndex_StackFrame* previousFrame)
{
    return PcpPayloadContext(parentNode, previousFrame);
}
