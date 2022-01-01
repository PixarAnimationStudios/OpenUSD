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


PXR_NAMESPACE_CLOSE_SCOPE

