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
#ifndef HD_DRAW_ITEM_H
#define HD_DRAW_ITEM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/rprimSharedData.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4f.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDrawItem
///
/// A draw item is a light-weight representation of an HdRprim's resources and 
/// material to be used for rendering. The visual representation (HdRepr) of an
/// HdRprim might require multiple draw items.
/// 
/// HdDrawItem(s) are created by the HdRprim (HdMesh, HdBasisCurve, ..) for each
/// HdRepr. The relevant compositional hierarchy is:
/// 
///  HdRprim
///  |
///  +--HdRepr(s)
///       |
///       +--HdDrawItem(s)
///
///  HdDrawItem(s) are consumed by HdRenderPass for its HdRprimCollection via
///  HdRenderIndex::GetDrawItems.
///
/// \note
/// Rendering backends may choose to specialize this class.
///
class HdDrawItem {
public:
    HF_MALLOC_TAG_NEW("new HdDrawItem");

    HD_API
    HdDrawItem(HdRprimSharedData const *sharedData);
    HD_API
    virtual ~HdDrawItem();

    HD_API
    SdfPath const &GetRprimID() const { return _sharedData->rprimID; }

    HD_API
    GfBBox3d const & GetBounds() const { return _sharedData->bounds; }

    HD_API
    GfRange3d const& GetExtent() const {
        return _sharedData->bounds.GetRange();
    }

    HD_API
    GfMatrix4d const& GetMatrix() const {
        return _sharedData->bounds.GetMatrix();
    }

    /// Returns a BufferRange of constant-Primvar.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetConstantPrimvarRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetConstantPrimvarIndex());
    }

    /// Returns the number of nested levels of instance primvars.
    HD_API
    int GetInstancePrimvarNumLevels() const {
        TF_VERIFY(_sharedData->instancerLevels != -1);
        return _sharedData->instancerLevels;
    }

    /// Returns a BufferRange of instance-Primvars at \p level
    /// the level is assigned to nested instancers in a bottom-up manner.
    ///
    /// example: (numLevels = 2)
    ///
    ///     instancerA         (level = 1)
    ///       |
    ///       +-- instancerB   (level = 0)
    ///             |
    ///             +-- mesh_prototype
    ///
    HD_API
    HdBufferArrayRangeSharedPtr const &GetInstancePrimvarRange(int level) const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetInstancePrimvarIndex(level));
    }

    /// Returns a BufferRange of instance-index indirection.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetInstanceIndexRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetInstanceIndexIndex());
    }

    /// Returns a BufferRange of element-Primvars.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetElementPrimvarRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetElementPrimvarIndex());
    }

    /// Returns a BufferArrayRange of topology.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetTopologyRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetTopologyIndex());
    }

    HD_API
    HdBufferArrayRangeSharedPtr const &GetTopologyVisibilityRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetTopologyVisibilityIndex());
    }

    /// Returns a BufferArrayRange of vertex-primvars.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetVertexPrimvarRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetVertexPrimvarIndex());
    }

    /// Returns a BufferArrayRange of face-varying primvars.
    HD_API
    HdBufferArrayRangeSharedPtr const &GetFaceVaryingPrimvarRange() const {
        return _sharedData->barContainer.Get(
            _drawingCoord.GetFaceVaryingPrimvarIndex());
    }

    HD_API
    HdDrawingCoord *GetDrawingCoord() {
        return &_drawingCoord;
    }

    /// Returns the authored visibility, expressed by the delegate.
    HD_API
    bool GetVisible() const { return _sharedData->visible; }

    /// Returns true if the drawItem has instancer.
    HD_API
    bool HasInstancer() const {
        TF_VERIFY(_sharedData->instancerLevels != -1);
        return (_sharedData->instancerLevels > 0);
    }

    /// Returns the hash of the versions of underlying buffers. When the
    /// hash changes, it means the drawing coord might have been reassigned,
    /// so any drawing coord caching buffer (e.g. indirect dispatch buffer)
    /// has to be rebuilt at the moment.
    /// Note that this value is a hash, not sequential.
    HD_API
    size_t GetBufferArraysHash() const;

    /// Tests the intersection with the view projection matrix.
    /// Returns true if this drawItem is in the frustum.
    ///
    /// XXX: Currently if this drawitem uses HW instancing, always returns true.
    HD_API
    bool IntersectsViewVolume(GfMatrix4d const &viewProjMatrix) const;

    HD_API
    friend std::ostream &operator <<(std::ostream &out, 
                                     const HdDrawItem& self);

protected:

    /// Returns the shared data
    HD_API
    HdRprimSharedData const *_GetSharedData() const {
        return _sharedData;
    }

private:
    // configuration of how to bundle the drawing coordinate for this draw item
    // out of BARs in sharedData
    HdDrawingCoord _drawingCoord;

    // pointer to shared data across reprs, owned by rprim:
    //    bufferArrayRanges, bounds, visibility
    HdRprimSharedData const *_sharedData;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_DRAW_ITEM_H
