//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

