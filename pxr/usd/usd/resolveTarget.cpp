//
// Copyright 2022 Pixar
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
#include "pxr/usd/usd/resolveTarget.h"

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

static SdfLayerRefPtrVector::const_iterator
_GetLayerIteratorInNode(const PcpNodeRef &node, const SdfLayerHandle &layer)
{
    // Null layer means we want the root layer of the node's layer stack.
    const SdfLayerRefPtrVector& layers = node.GetLayerStack()->GetLayers();
    if (!layer) {
        return layers.begin();
    }

    SdfLayerRefPtrVector::const_iterator layerIt = layers.begin();
    for (; layerIt != layers.end(); ++layerIt) {
        if (*layerIt == layer) {
            return layerIt;
        }
    }

    // We expect the call sites that can construct resolve targets to only 
    // provide layers that are in the node's layer stack.
    TF_CODING_ERROR("Layer not present in node");
    return layers.begin();
}

UsdResolveTarget::UsdResolveTarget(
    const std::shared_ptr<PcpPrimIndex> &index, 
    const PcpNodeRef &node, 
    const SdfLayerHandle &layer) : 
    _expandedPrimIndex(index),
    _nodeRange(index->GetNodeRange())
{
    // Always stop at the end of the prim index graph since no stop node is 
    // provided.
    _stopNodeIt = _nodeRange.second;

    _startNodeIt = index->GetNodeIteratorAtNode(node);
    if (_startNodeIt != _nodeRange.second) {
        _startLayerIt = _GetLayerIteratorInNode(*_startNodeIt, layer);
    }
}

UsdResolveTarget::UsdResolveTarget(
    const std::shared_ptr<PcpPrimIndex> &index, 
    const PcpNodeRef &node, 
    const SdfLayerHandle &layer,
    const PcpNodeRef &stopNode, 
    const SdfLayerHandle &stopLayer) :
    _expandedPrimIndex(index),
    _nodeRange(index->GetNodeRange())
{
    _stopNodeIt = stopNode ? 
        index->GetNodeIteratorAtNode(stopNode) : _nodeRange.second;
    if (_stopNodeIt != _nodeRange.second) {
        _stopLayerIt = _GetLayerIteratorInNode(*_stopNodeIt, stopLayer);
    }

    _startNodeIt = index->GetNodeIteratorAtNode(node);
    if (_startNodeIt != _nodeRange.second) {
        _startLayerIt = _GetLayerIteratorInNode(*_startNodeIt, layer);
    }
}

PcpNodeRef 
UsdResolveTarget::GetStartNode() const {
    return _startNodeIt != _nodeRange.second ? *_startNodeIt : PcpNodeRef();
}

PcpNodeRef 
UsdResolveTarget::GetStopNode() const {
    return _stopNodeIt != _nodeRange.second ? *_stopNodeIt : PcpNodeRef();
}

SdfLayerHandle 
UsdResolveTarget::GetStartLayer() const {
    return _startNodeIt != _nodeRange.second ? *_startLayerIt : nullptr;
}

SdfLayerHandle 
UsdResolveTarget::GetStopLayer() const {
    return _stopNodeIt != _nodeRange.second ? *_stopLayerIt : nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

