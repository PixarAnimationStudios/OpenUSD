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
#ifndef USDIMAGINGGL_HYDRAMATERIALADAPTER_H
#define USDIMAGINGGL_HYDRAMATERIALADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/imaging/hd/materialParam.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingGLHydraMaterialAdapter
/// \brief Provides information that can be used to generate a surface shader in
/// hydra.
class UsdImagingGLHydraMaterialAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdImagingGLHydraMaterialAdapter()
        : UsdImagingPrimAdapter()
    {}

    USDIMAGINGGL_API
    virtual ~UsdImagingGLHydraMaterialAdapter();

    USDIMAGINGGL_API
    virtual SdfPath Populate(
            UsdPrim const& prim,
            UsdImagingIndexProxy* index,
            UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGINGGL_API
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const override;

    USDIMAGINGGL_API
    virtual bool IsPopulatedIndirectly() override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDIMAGINGGL_API
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL) const override;


    /// Thread Safe.
    USDIMAGINGGL_API
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    /// Returns a bit mask of attributes to be udpated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    USDIMAGINGGL_API
    virtual HdDirtyBits ProcessPropertyChange(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            TfToken const& propertyName) override;

    USDIMAGINGGL_API
    virtual void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    virtual void MarkMaterialDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index) override;


    // ---------------------------------------------------------------------- //
    /// \name Texture resources
    // ---------------------------------------------------------------------- //

    virtual HdTextureResource::ID
    GetTextureResourceID(UsdPrim const& usdPrim, 
                         SdfPath const &id, 
                         UsdTimeCode time, 
                         size_t salt) const override;

    virtual HdTextureResourceSharedPtr
    GetTextureResource(UsdPrim const& usdPrim, 
                       SdfPath const &id, 
                       UsdTimeCode time) const override;

protected:
    USDIMAGINGGL_API
    virtual void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) final;

private:
    /// \brief Returns the source string for the specified shader
    /// terminal for the shader \p prim.
    /// 
    /// This obtains the shading source.
    std::string _GetShaderSource(UsdPrim const& prim,
                                 TfToken const& shaderType,
                                 VtDictionary * metadataOut = nullptr) const;

    /// \brief Returns the root "surface" shader prim for the material, by 
    /// traversing which the entire hydra shading network can be discovered.
    /// 
    /// Returns an invalid prim if the material does not have a 'surface' 
    /// shader.
    UsdPrim _GetSurfaceShaderPrim(const UsdShadeMaterial &material) const;

    /// \brief Returns the root displacement shader prim for the material.
    /// 
    /// Returns an invalid prim if the material does not have a 'displacement' 
    /// shader.
    UsdPrim _GetDisplacementShaderPrim(const UsdShadeMaterial &material) const;

    /// \brief Populates the information in the material graph
    /// (identified by \c SdfPath objects) that this \p materialPrim uses.
    /// 
    /// Returns false if the material prim has an invalid material graph.
    bool _GatherMaterialData(
        UsdPrim const& materialPrim,
        UsdPrim *shaderPrim,
        UsdPrim *displacementShaderPrim,
        SdfPathVector *textureIDs,
        TfTokenVector *primvars,
        HdMaterialParamVector *materialParams) const;

    /// \brief Returns the information in the material graph
    /// (identified by \c SdfPath objects) that this \p shaderPrim uses.
    void _WalkShaderNetwork(
        UsdPrim const& shaderPrim,
        SdfPathVector *textureIDs,
        TfTokenVector *primvars,
        HdMaterialParamVector *materialParams) const;

    /// \brief Returns the information in a legacy material graph
    /// (identified by \c SdfPath objects) that this \p shaderPrim uses.
    void _WalkShaderNetworkDeprecated(
        UsdPrim const &shaderPrim,
        SdfPathVector *textureIDs,
        TfTokenVector *primvars,
        HdMaterialParamVector *materialParams) const;

    /// \brief Returns the value of param \p paramName for \p shaderPrim.
    VtValue _GetMaterialParamValue(UsdPrim const& shaderPrim,
                                   TfToken const& paramName,
                                   UsdTimeCode time) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_HYDRAMATERIALADAPTER_H
