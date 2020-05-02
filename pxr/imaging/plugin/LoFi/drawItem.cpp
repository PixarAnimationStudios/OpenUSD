//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/drawItem.h"

#include "pxr/base/gf/frustum.h"

#include <boost/functional/hash.hpp>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawItem::LoFiDrawItem(HdRprimSharedData const *sharedData)
    : HdDrawItem(sharedData)
    , _program(NULL)
    , _dualMesh(NULL)
{
    HF_MALLOC_TAG_FUNCTION();
}

LoFiDrawItem::~LoFiDrawItem()
{
    /*NOTHING*/
}

size_t
LoFiDrawItem::_GetBufferArraysHash() const
{
    return _hash;
}

void 
LoFiDrawItem::ClearSilhouettes()
{
    if(_dualMesh)
    {
        _dualMesh->ClearSilhouettes();
    }
}
void 
LoFiDrawItem::FindSilhouettes(const GfMatrix4f& viewMatrix)
{
    if(_dualMesh)
    {

    }
}

void 
LoFiDrawItem::PopulateInstancesXforms(const VtArray<GfMatrix4d>& xforms)
{
    size_t numInstances = xforms.size();
    _instancesXform.resize(numInstances);
    for(int i=0;i<numInstances;++i)
    {
        _instancesXform[i] = GfMatrix4f(xforms[i]);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

