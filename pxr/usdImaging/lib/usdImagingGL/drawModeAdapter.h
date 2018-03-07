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
#ifndef USDIMAGINGGL_DRAW_MODE_ADAPTER_H
#define USDIMAGINGGL_DRAW_MODE_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usd/usdGeom/xformCache.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingGLDrawModeAdapter
///
/// Delegate support for the drawMode attribute on UsdGeomModelAPI.
///
class UsdImagingGLDrawModeAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdImagingGLDrawModeAdapter()
        : UsdImagingPrimAdapter()
    {}

    USDIMAGINGGL_API
    virtual ~UsdImagingGLDrawModeAdapter();

    /// Called to populate the RenderIndex for this UsdPrim. The adapter is
    /// expected to create one or more Rprims in the render index using the
    /// given proxy.
    virtual SdfPath Populate(UsdPrim const& prim,
                UsdImagingIndexProxy* index,
                UsdImagingInstancerContext const* instancerContext = NULL);

    // If the draw mode adapter is applied to a prim, it cuts off traversal of
    // that prim's subtree.
    virtual bool ShouldCullChildren(UsdPrim const& prim) {
        return true;
    }

    // Because draw mode can change usdImaging topology, we need to handle
    // render index compatibility at a later point than adapter lookup.
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const {
        return true;
    }

    virtual bool IsNativeInstanceable(UsdPrim const& prim) { return true; }

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    USDIMAGINGGL_API
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL);

    USDIMAGINGGL_API
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL);

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    USDIMAGINGGL_API
    virtual HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath, 
                                              TfToken const& property);

    USDIMAGINGGL_API
    virtual void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index);

    USDIMAGINGGL_API
    virtual void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index);

    USDIMAGINGGL_API
    virtual void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index);

    // ---------------------------------------------------------------------- //
    /// \name Texture resources
    // ---------------------------------------------------------------------- //

    virtual HdTextureResource::ID
    GetTextureResourceID(UsdPrim const& usdPrim, SdfPath const &id, UsdTimeCode time, size_t salt) const override;

    virtual HdTextureResourceSharedPtr
    GetTextureResource(UsdPrim const& usdPrim, SdfPath const &id, UsdTimeCode time) const override;

protected:
    USDIMAGINGGL_API
    virtual void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index);

private:
    // For cards rendering, check if we're rendering any faces with 0 area;
    // if so, issue a warning.
    void _SanityCheckFaceSizes(SdfPath const& cachePath,
                               GfRange3d const& extents, uint8_t axes_mask);

    // Check whether the given cachePath is a path to the draw mode material.
    bool _IsMaterialPath(SdfPath const& path);
    // Check whether the given cachePath is a path to a draw mode texture.

    bool _IsTexturePath(SdfPath const& path);
    // Computes the extents of the given prim, using UsdGeomBBoxCache.
    // The extents are computed at UsdTimeCode::EarliestTime() (and are not
    // animated), and they are computed for purposes default/proxy/render.
    GfRange3d _ComputeExtent(UsdPrim const& prim);

    // Returns the UsdGeomImagable "purpose" for this prim, including any
    // inherited purpose. Inherited values are strongest.
    TfToken _GetPurpose(UsdPrim const& prim, UsdTimeCode time);

    // Returns the draw mode surface shader.
    std::string _GetSurfaceShaderSource();

    // Generate geometry for "origin" draw mode.
    void _GenerateOriginGeometry(VtValue* topo, VtValue* points,
                                 GfRange3d const& extents);

    // Generate geometry for "bounds" draw mode.
    void _GenerateBoundsGeometry(VtValue* topo, VtValue* points,
                                 GfRange3d const& extents);

    // Generate geometry for "cards" draw mode, with cardGeometry "cross".
    void _GenerateCardsCrossGeometry(VtValue* topo, VtValue* points,
            GfRange3d const& extents, uint8_t axes_mask);

    // Generate geometry for "cards" draw mode, with cardGeometry "box".
    void _GenerateCardsBoxGeometry(VtValue* topo, VtValue* points,
            GfRange3d const& extents, uint8_t axes_mask);

    // Generate geometry for "cards" draw mode, with cardGeometry "fromTexture".
    void _GenerateCardsFromTextureGeometry(VtValue* topo, VtValue* points,
            VtValue* uv, VtValue* assign, GfRange3d* extents,
            UsdPrim const& prim);

    // Given an asset attribute pointing to a texture, pull the "worldtoscreen"
    // matrix out of image metadata.
    bool _GetMatrixFromImageMetadata(UsdAttribute const& attr, GfMatrix4d* mat);

    // Generate texture coordinates for cards "cross"/"box" mode.
    void _GenerateTextureCoordinates(VtValue* uv, VtValue* assign,
                                     uint8_t axes_mask);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_DRAW_MODE_ADAPTER_H
