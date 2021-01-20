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
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/bufferArrayRange.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/hash.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

static size_t _GetVersion(HdBufferArrayRangeSharedPtr const &bar)
{
    if (bar) {
        return bar->GetVersion();
    } else {
        return 0;
    }
}

static size_t _GetElementOffset(HdBufferArrayRangeSharedPtr const &bar)
{
    if (bar) {
        return bar->GetElementOffset();
    } else {
        return 0;
    }
}

HdDrawItem::HdDrawItem(HdRprimSharedData const *sharedData)
    : _sharedData(sharedData)
{
    HF_MALLOC_TAG_FUNCTION();
}

HdDrawItem::~HdDrawItem()
{
    /*NOTHING*/
}

template <class HashState>
void
TfHashAppend(HashState &h, HdDrawItem const &di)
{
    h.Append(_GetVersion(di.GetTopologyRange()));
    h.Append(_GetVersion(di.GetConstantPrimvarRange()));
    h.Append(_GetVersion(di.GetVertexPrimvarRange()));
    h.Append(_GetVersion(di.GetVaryingPrimvarRange()));
    h.Append(_GetVersion(di.GetElementPrimvarRange()));
    h.Append(_GetVersion(di.GetFaceVaryingPrimvarRange()));
    h.Append(_GetVersion(di.GetTopologyVisibilityRange()));

    int const instancerNumLevels = di.GetInstancePrimvarNumLevels();
    for (int i = 0; i < instancerNumLevels; ++i) {
        h.Append(_GetVersion(di.GetInstancePrimvarRange(i)));
    }
    h.Append(_GetVersion(di.GetInstanceIndexRange()));

    h.Append(di._GetBufferArraysHash());
}

size_t
HdDrawItem::GetBufferArraysHash() const
{
    return TfHash()(*this);
}

size_t
HdDrawItem::GetElementOffsetsHash() const
{
    size_t hash = TfHash::Combine(
        _GetElementOffset(GetTopologyRange()),
        _GetElementOffset(GetConstantPrimvarRange()),
        _GetElementOffset(GetVertexPrimvarRange()),
        _GetElementOffset(GetVaryingPrimvarRange()),
        _GetElementOffset(GetElementPrimvarRange()),
        _GetElementOffset(GetFaceVaryingPrimvarRange()),
        _GetElementOffset(GetTopologyVisibilityRange()));
    
    int const instancerNumLevels = GetInstancePrimvarNumLevels();
    for (int i = 0; i < instancerNumLevels; ++i) {
        hash = TfHash::Combine(hash,
                               _GetElementOffset(GetInstancePrimvarRange(i)));
    }
    hash = TfHash::Combine(hash,
                           _GetElementOffset(GetInstanceIndexRange()),
                           _GetElementOffsetsHash());

    return hash;
}

bool
HdDrawItem::IntersectsViewVolume(GfMatrix4d const &viewProjMatrix) const
{
    if (GetInstanceIndexRange()) {
        // XXX: need to test intersections of the bound of all instances.
        return true;
    } else {
        return GfFrustum::IntersectsViewVolume(GetBounds(), viewProjMatrix);
    }
}

HD_API
std::ostream &operator <<(std::ostream &out, 
                          const HdDrawItem& self) {
    out << "Draw Item:\n";
    out << "    Bound: "    << self._sharedData->bounds << "\n";
    out << "    Visible: "  << self._sharedData->visible << "\n";
    if (self.GetTopologyRange()) {
        out << "    Topology:\n";
        out << "        numElements=" << self.GetTopologyRange()->GetNumElements() << "\n";
        out << *self.GetTopologyRange();
    }
    if (self.GetConstantPrimvarRange()) {
        out << "    Constant Primvars:\n";
        out << *self.GetConstantPrimvarRange();
    }
    if (self.GetElementPrimvarRange()) {
        out << "    Element Primvars:\n";
        out << "        numElements=" << self.GetElementPrimvarRange()->GetNumElements() << "\n";
        out << *self.GetElementPrimvarRange();
    }
    if (self.GetVertexPrimvarRange()) {
        out << "    Vertex Primvars:\n";
        out << "        numElements=" << self.GetVertexPrimvarRange()->GetNumElements() << "\n";
        out << *self.GetVertexPrimvarRange();
    }
    if (self.GetVaryingPrimvarRange()) {
        out << "    Varying Primvars:\n";
        out << "        numElements=" << self.GetVaryingPrimvarRange()->GetNumElements() << "\n";
        out << *self.GetVaryingPrimvarRange();
    }
    if (self.GetFaceVaryingPrimvarRange()) {
        out << "    Fvar Primvars:\n";
        out << "        numElements=" << self.GetFaceVaryingPrimvarRange()->GetNumElements() << "\n";
        out << *self.GetFaceVaryingPrimvarRange();
    }
    if (self.GetTopologyVisibilityRange()) {
        out << "    Topology visibility:\n";
        out << *self.GetTopologyVisibilityRange();
    }
    return out;
}

size_t
HdDrawItem::_GetBufferArraysHash() const
{
    return 0;
}

size_t
HdDrawItem::_GetElementOffsetsHash() const
{
    return 0;
}


PXR_NAMESPACE_CLOSE_SCOPE

