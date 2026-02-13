//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/resolver.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/resolveTarget.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"

PXR_NAMESPACE_OPEN_SCOPE

Usd_Resolver::Usd_Resolver(const PcpPrimIndex* index,
                           bool skipEmptyNodes,
                           const UsdResolveInfo *resolveInfo)
    : _index(index)
    , _skipEmptyNodes(skipEmptyNodes)
    , _resolveTarget(nullptr)
{
    PcpNodeRange range = _index->GetNodeRange();
    _curNode = resolveInfo && resolveInfo->_node
        ? std::find(range.first, range.second, resolveInfo->_node)
        : range.first;
    _endNode = range.second;

    _SkipEmptyNodes();

    // The entire stage may be empty, so we need to check IsValid here.
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers = 
            _curNode->GetLayerStack()->GetLayers();
        _curLayer = resolveInfo && resolveInfo->_layer
            ? std::find(layers.begin(), layers.end(), resolveInfo->_layer)
            : layers.begin();
        _endLayer = layers.end();
    }
}

Usd_Resolver::Usd_Resolver(const UsdResolveTarget *resolveTarget,
                           bool skipEmptyNodes,
                           const UsdResolveInfo *resolveInfo)
    : _skipEmptyNodes(skipEmptyNodes)
    , _resolveTarget(resolveTarget)
{
    if (!TF_VERIFY(_resolveTarget)) {
        _index = nullptr;
        return;
    }

    _index = _resolveTarget->GetPrimIndex();
    _curNode = resolveInfo && resolveInfo->_node
        ? std::find(_resolveTarget->_startNodeIt,
                    _index->GetNodeRange().second, resolveInfo->_node)
        : _resolveTarget->_startNodeIt;
    _endNode = _index->GetNodeRange().second;

    // If the resolve target provided a node to stop at before the end of the
    // prim index graph, we have to figure out the end iterators.
    if (_resolveTarget->_stopNodeIt != _endNode) {
        // First assume we end as soon as we reach the stop node.
        _endNode = _resolveTarget->_stopNodeIt;

        // Check if the stop layer is past the beginning of the stop node layer
        // stack. If so, we'll need to iterate into the stop node to catch those
        // layers, so move the end node forward.
        const SdfLayerRefPtrVector& layers = 
            _resolveTarget->_stopNodeIt->GetLayerStack()->GetLayers();
        if (_resolveTarget->_stopLayerIt != layers.begin()) {
            ++_endNode;
        }
    }

    _SkipEmptyNodes();

    // The prim index may be empty within the resolve target range, so we need
    // to check IsValid here.
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers = 
            _curNode->GetLayerStack()->GetLayers();

        // If we were given a resolveInfo with a layer, start there.  Otherwise
        // if we haven't skipped past the resolve target's start node, start
        // with the resolve target's start layer.
        if (resolveInfo && resolveInfo->_layer) {
            _curLayer = std::find(layers.begin(), layers.end(),
                                  resolveInfo->_layer);
        }
        else {
            if (_curNode == _resolveTarget->_startNodeIt) {
                _curLayer = _resolveTarget->_startLayerIt;
            } else {
                _curLayer = layers.begin();
            }
        }

        // If we reached the "stop at node" (and the resolver is still valid),
        // the "stop at layer" determines what the end layer is.
        if (_curNode == _resolveTarget->_stopNodeIt) {
            _endLayer = _resolveTarget->_stopLayerIt;
        } else {
            _endLayer = layers.end();
        }
    }
}

void
Usd_Resolver::_SkipEmptyNodes()
{
    if (_skipEmptyNodes) {
        for (; IsValid() && (!_curNode->HasSpecs() ||
                             _curNode->IsInert()); ++_curNode) {
            // do nothing.
        }
    } else {
        for (; IsValid() && _curNode->IsInert(); ++_curNode) {
            // do nothing.
        }
    }
}

void 
Usd_Resolver::NextNode()
{
    ++_curNode;
    _SkipEmptyNodes();
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers =
            _curNode->GetLayerStack()->GetLayers();
        _curLayer = layers.begin();

        // If we reached the "stop at node" (and the resolver is still valid),
        // the "stop at layer" determines what the end layer is.
        if (_resolveTarget && _curNode == _resolveTarget->_stopNodeIt) {
            _endLayer = _resolveTarget->_stopLayerIt;
        } else {
            _endLayer = layers.end();
        }
    }
}

bool 
Usd_Resolver::NextLayer() {
    if (++_curLayer == _endLayer) {
        // We hit the last layer in this LayerStack, move on to the next node.
        NextNode();
        return true;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

