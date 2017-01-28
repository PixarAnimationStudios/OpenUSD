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
    PcpNodeRange _range = _index->GetNodeRange();
    _curNode = _range.first;
    _lastNode = _range.second;

    _SkipEmptyNodes();

    // The entire stage may be empty, so we need to check IsValid here.
    if (IsValid()) {
        const SdfLayerRefPtrVector& layers = _curNode->GetLayerStack()->GetLayers();
        _curLayer = layers.begin();
        _lastLayer = layers.end();
    }
}

void
Usd_Resolver::_SkipEmptyNodes()
{
    while (IsValid() &&
           ((_skipEmptyNodes && !_curNode->HasSpecs()) ||
            _curNode->IsInert())) {
        _curNode++;
    }
}

Usd_Resolver::Usd_Resolver(const PcpPrimIndex* index, bool skipEmptyNodes) 
    : _index(index)
    , _skipEmptyNodes(skipEmptyNodes)
{
    _Init();
}

PcpNodeRef
Usd_Resolver::GetNode() const
{
    if (!IsValid())
        return PcpNodeRef();
    return *_curNode;
}

const SdfLayerRefPtr&
Usd_Resolver::GetLayer() const
{
    if (!IsValid()) {
        static const SdfLayerRefPtr _NULL_LAYER;
        return _NULL_LAYER;
    }
    return *_curLayer;
}

const SdfPath&
Usd_Resolver::GetLocalPath() const
{
    if (!IsValid())
        return SdfPath::EmptyPath();
    return _curNode->GetPath(); 
}

const PcpPrimIndex*
Usd_Resolver::GetPrimIndex() const
{
    return _index; 
}

void 
Usd_Resolver::NextNode()
{
    if (IsValid()) {
        ++_curNode;
        _SkipEmptyNodes();
        if (IsValid()) {
            const SdfLayerRefPtrVector& layers =
                _curNode->GetLayerStack()->GetLayers();
            _curLayer = layers.begin();
            _lastLayer = layers.end();
        }
    }
}

bool 
Usd_Resolver::NextLayer() {
    if (!IsValid())
        return true;

    if (++_curLayer == _lastLayer) {
        // We hit the last layer in this LayerStack, move on to the next node.
        NextNode();
        return true;
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE

