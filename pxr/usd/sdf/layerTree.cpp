//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
