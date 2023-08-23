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
#ifndef PXR_IMAGING_HD_DRAW_ITEM_H
#define PXR_IMAGING_HD_DRAW_ITEM_H

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
class HdDrawItem
{
public:
    HF_MALLOC_TAG_NEW("new HdDrawItem");

    HD_API
    HdDrawItem(HdRprimSharedData const *sharedData);

    HD_API
    virtual ~HdDrawItem();

    SdfPath const &GetRprimID() const { return _sharedData->rprimID; }

    GfBBox3d const & GetBounds() const { return _sharedData->bounds; }

    GfRange3d const& GetExtent() const {
        return _sharedData->bounds.GetRange();
    }

    GfMatrix4d const& GetMatrix() const {
        return _sharedData->bounds.GetMatrix();
    }

    HdDrawingCoord *GetDrawingCoord() {
        return &_drawingCoord;
    }

    /// Returns the authored visibility, expressed by the delegate.
    bool GetVisible() const { return _sharedData->visible; }

    TfToken const& GetMaterialTag() const {
        return _materialTag;
    }

    void SetMaterialTag(TfToken const &materialTag) {
        _materialTag = materialTag;
    }

protected:
    /// Returns the drawingCoord
    HdDrawingCoord const &_GetDrawingCoord() const {
        return _drawingCoord;
    }

    /// Returns the shared data
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

    /// The materialTag allows the draw items of rprims to be organized into 
    /// different collections based on properties of the prim's material.
    /// E.g. A renderer may wish to organize opaque and translucent prims 
    /// into different collections so they can be rendered seperately.
    TfToken _materialTag;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_DRAW_ITEM_H
