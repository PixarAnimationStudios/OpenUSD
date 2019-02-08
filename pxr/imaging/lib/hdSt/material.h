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
#ifndef HDST_MATERIAL_H
#define HDST_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hf/perfLog.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef boost::shared_ptr<class HdStSurfaceShader> HdStSurfaceShaderSharedPtr;
typedef boost::shared_ptr<class HdStTextureResource> HdStTextureResourceSharedPtr;
typedef std::vector<HdStTextureResourceSharedPtr>
                                HdStTextureResourceSharedPtrVector;

class HioGlslfx;

class HdStMaterial final: public HdMaterial {
public:
    HF_MALLOC_TAG_NEW("new HdStMaterial");

    HDST_API
    HdStMaterial(SdfPath const& id);
    HDST_API
    virtual ~HdStMaterial();

    /// Synchronizes state from the delegate to this object.
    HDST_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Causes the shader to be reloaded.
    HDST_API
    virtual void Reload() override;

    /// Obtains the render delegate specific representation of the shader.
    HDST_API
    HdStShaderCodeSharedPtr GetShaderCode() const;

    /// Obtain the source code for the Surface Shader for this prim from
    /// the scene delegate.
    inline std::string GetSurfaceShaderSource(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the source code for the Displacement Shader for this prim from
    /// the scene delegate.
    inline std::string GetDisplacementShaderSource(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the collection of material param descriptions for this prim from
    /// the scene delegate.
    inline HdMaterialParamVector GetMaterialParams(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the value of the specified material param for this prim from the
    /// scene delegate.
    inline VtValue GetMaterialParamValue(HdSceneDelegate* sceneDelegate,
                                         TfToken const &paramName) const;

    /// Obtains the metadata dictionary for the material.
    inline VtDictionary GetMaterialMetadata(
        HdSceneDelegate* sceneDelegate) const;

    /// Obtain the scene delegates's globally unique id for the texture
    /// resource identified by textureId.
    inline HdTextureResource::ID GetTextureResourceID(
        HdSceneDelegate* sceneDelegate,
        SdfPath const& textureId) const;

    /// Summary flag. Returns true if the material is bound to one or more
    /// textures and any of those textures is a ptex texture.
    /// If no textures are bound or all textures are uv textures, then
    /// the method returns false.
    inline bool HasPtex() const;

    /// Returns true if the material specifies limit surface evaluation.
    inline bool HasLimitSurfaceEvaluation() const;

    // Returns true if the material has a displacement terminal.
    inline bool HasDisplacement() const;

    /// Replaces the shader code object with an externally created one
    /// Used to set the fallback shader for prim.
    /// This class takes ownership of the passed in object.
    HDST_API
    void SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode);

private:
    HdStTextureResourceSharedPtr
    _GetTextureResource(HdSceneDelegate *sceneDelegate,
                        HdMaterialParam const &param);

    bool
    _GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const;

    void _InitFallbackShader();

    static HioGlslfx                  *_fallbackSurfaceShader;

    HdStSurfaceShaderSharedPtr         _surfaceShader;
    HdStTextureResourceSharedPtrVector _fallbackTextureResources;

    bool                               _hasPtex : 1;
    bool                               _hasLimitSurfaceEvaluation : 1;
    bool                               _hasDisplacement : 1;
};

inline std::string
HdStMaterial::GetSurfaceShaderSource(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetSurfaceShaderSource(GetId());
}

inline std::string
HdStMaterial::GetDisplacementShaderSource(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetDisplacementShaderSource(GetId());
}

inline HdMaterialParamVector
HdStMaterial::GetMaterialParams(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetMaterialParams(GetId());
}

inline VtValue
HdStMaterial::GetMaterialParamValue(HdSceneDelegate* sceneDelegate,
                                  TfToken const &paramName) const
{
    return sceneDelegate->GetMaterialParamValue(GetId(), paramName);
}

inline VtDictionary
HdStMaterial::GetMaterialMetadata(HdSceneDelegate* sceneDelegate) const
{
    return sceneDelegate->GetMaterialMetadata(GetId());
}

inline HdTextureResource::ID
HdStMaterial::GetTextureResourceID(HdSceneDelegate* sceneDelegate,
                               SdfPath const& textureId) const
{
    return sceneDelegate->GetTextureResourceID(textureId);
}

inline bool HdStMaterial::HasPtex() const
{
    return _hasPtex;
}

inline bool HdStMaterial::HasLimitSurfaceEvaluation() const
{
    return _hasLimitSurfaceEvaluation;
}

inline bool HdStMaterial::HasDisplacement() const
{
    return _hasDisplacement;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_MATERIAL_H
