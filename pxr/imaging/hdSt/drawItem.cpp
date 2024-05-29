//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"

#include "pxr/imaging/hd/bufferArrayRange.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStDrawItem::HdStDrawItem(HdRprimSharedData const *sharedData)
    : HdDrawItem(sharedData)
    , _materialIsFinal(false)
{
    HF_MALLOC_TAG_FUNCTION();
}

HdStDrawItem::~HdStDrawItem() = default;

static size_t
_GetVersion(HdBufferArrayRangeSharedPtr const &bar)
{
    return bar ? bar->GetVersion() : 0;
}

static size_t
_GetElementOffset(HdBufferArrayRangeSharedPtr const &bar)
{
    return bar ? bar->GetElementOffset() : 0;
}

static HdBufferArrayRangeSharedPtr
_GetShaderBar(HdSt_MaterialNetworkShaderSharedPtr const &shader)
{
    return shader ? shader->GetShaderData() : nullptr;
}

size_t
HdStDrawItem::GetBufferArraysHash() const
{
    size_t hash = TfHash::Combine(
        _GetVersion(GetConstantPrimvarRange()),
        _GetVersion(GetElementPrimvarRange()),
        _GetVersion(GetVertexPrimvarRange()),
        _GetVersion(GetVaryingPrimvarRange()),
        _GetVersion(GetFaceVaryingPrimvarRange()),
        _GetVersion(GetTopologyRange()),
        _GetVersion(GetTopologyVisibilityRange()),
        _GetVersion(GetInstanceIndexRange()),
        _GetVersion(_GetShaderBar(GetMaterialNetworkShader())));

    int const instancerNumLevels = GetInstancePrimvarNumLevels();
    for (int i = 0; i < instancerNumLevels; ++i) {
        hash = TfHash::Combine(hash,
                _GetVersion(GetInstancePrimvarRange(i)));
    }

    return hash;
}

size_t
HdStDrawItem::GetElementOffsetsHash() const
{
    size_t hash = TfHash::Combine(
        _GetElementOffset(GetConstantPrimvarRange()),
        _GetElementOffset(GetElementPrimvarRange()),
        _GetElementOffset(GetVertexPrimvarRange()),
        _GetElementOffset(GetVaryingPrimvarRange()),
        _GetElementOffset(GetFaceVaryingPrimvarRange()),
        _GetElementOffset(GetTopologyRange()),
        _GetElementOffset(GetTopologyVisibilityRange()),
        _GetElementOffset(GetInstanceIndexRange()),
        _GetElementOffset(_GetShaderBar(GetMaterialNetworkShader())));

    int const instancerNumLevels = GetInstancePrimvarNumLevels();
    for (int i = 0; i < instancerNumLevels; ++i) {
        hash = TfHash::Combine(hash,
                _GetElementOffset(GetInstancePrimvarRange(i)));
    }

    return hash;
}

bool
HdStDrawItem::IntersectsViewVolume(GfMatrix4d const &viewProjMatrix) const
{
    if (GetInstanceIndexRange()) {
        // XXX: need to test intersections of the bound of all instances.
        return true;
    } else {
        return GfFrustum::IntersectsViewVolume(GetBounds(), viewProjMatrix);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

