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
#include "pxr/usd/usd/resolver.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/resolveTarget.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"

PXR_NAMESPACE_OPEN_SCOPE

Usd_Resolver::Usd_Resolver(const PcpPrimIndex* index, bool skipEmptyNodes) 
     :_index(index)
     , _skipEmptyNodes(skipEmptyNodes)
     , _resolveTarget(nullptr)
{
    PcpNodeRange range = _index->GetNodeRange();
    _curNode = range.first;
    _endNode = range.second;

    _SkipEmptyNodes();

    // The entire stage may be empty, so we need to check IsValid here.
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers = 
            _curNode->GetLayerStack()->GetLayers();
        _curLayer = layers.begin();
        _endLayer = layers.end();
    }
}

Usd_Resolver::Usd_Resolver(
    const UsdResolveTarget *resolveTarget, bool skipEmptyNodes) 
    : _skipEmptyNodes(skipEmptyNodes)
    , _resolveTarget(resolveTarget)
{
    if (!TF_VERIFY(_resolveTarget)) {
        _index = nullptr;
        return;
    }

    _index = _resolveTarget->GetPrimIndex();
    _curNode = _resolveTarget->_startNodeIt;
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

        // If we haven't skipped past the resolve target's start node, start
        // with the resolve target's start layer.
        if (_curNode == _resolveTarget->_startNodeIt) {
            _curLayer = _resolveTarget->_startLayerIt;
        } else {
            _curLayer = layers.begin();
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

