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

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"

PXR_NAMESPACE_OPEN_SCOPE


void
Usd_Resolver::_Init() {
    PcpNodeRange range = _index->GetNodeRange();
    _curNode = range.first;
    _endNode = range.second;

    _SkipEmptyNodes();

    // The entire stage may be empty, so we need to check IsValid here.
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers = _curNode->GetLayerStack()->GetLayers();
        _curLayer = layers.begin();
        _endLayer = layers.end();
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

Usd_Resolver::Usd_Resolver(const PcpPrimIndex* index, bool skipEmptyNodes) 
    : _index(index)
    , _skipEmptyNodes(skipEmptyNodes)
{
    _Init();
}

size_t 
Usd_Resolver::GetLayerStackIndex() const 
{
    return std::distance(
        _curNode->GetLayerStack()->GetLayers().begin(), _curLayer);
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
        _endLayer = layers.end();
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

