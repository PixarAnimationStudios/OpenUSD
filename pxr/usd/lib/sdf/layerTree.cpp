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
/// \file LayerTree.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerTree.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// SdfLayerTree

SdfLayerTreeHandle
SdfLayerTree::New( const SdfLayerHandle & layer,
                   const SdfLayerTreeHandleVector & childTrees,
                   const SdfLayerOffset & cumulativeOffset )
{
    return TfCreateRefPtr( new SdfLayerTree(layer, childTrees,
                                                cumulativeOffset) );
}

SdfLayerTree::SdfLayerTree( const SdfLayerHandle & layer,
                            const SdfLayerTreeHandleVector & childTrees,
                            const SdfLayerOffset & cumulativeOffset ) :
    _layer(layer),
    _offset(cumulativeOffset),
    _childTrees(childTrees)
{
    // Do nothing
}

const SdfLayerHandle &
SdfLayerTree::GetLayer() const
{
    return _layer;
}

const SdfLayerOffset &
SdfLayerTree::GetOffset() const
{
    return _offset;
}

const SdfLayerTreeHandleVector & 
SdfLayerTree::GetChildTrees() const
{
    return _childTrees;
}

PXR_NAMESPACE_CLOSE_SCOPE
