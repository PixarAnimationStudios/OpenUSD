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
#ifndef PXR_IMAGING_HD_ST_MATERIAL_H
#define PXR_IMAGING_HD_ST_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hf/perfLog.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef boost::shared_ptr<class HdStSurfaceShader> HdStSurfaceShaderSharedPtr;
typedef boost::shared_ptr<class HdStTextureResource> HdStTextureResourceSharedPtr;
typedef boost::shared_ptr<class HdStTextureResourceHandle> HdStTextureResourceHandleSharedPtr;
typedef std::vector<HdStTextureResourceHandleSharedPtr>
                                HdStTextureResourceHandleSharedPtrVector;

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

    // Returns the material's render pass tag.
    inline const TfToken& GetMaterialTag() const;

    /// Replaces the shader code object with an externally created one
    /// Used to set the fallback shader for prim.
    /// This class takes ownership of the passed in object.
    HDST_API
    void SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode);

private:
    HdStTextureResourceHandleSharedPtr
    _GetTextureResourceHandle(HdSceneDelegate *sceneDelegate,
                              HdMaterialParam const &param);

    bool
    _GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const;

    void _InitFallbackShader();

    static HioGlslfx *_fallbackGlslfx;

    HdStSurfaceShaderSharedPtr _surfaceShader;

    // Holds fallback textures if a texture cannot be found, but also holds
    // texture we discovered inside a material network that could not be found
    // in the resource registry (no Bprim inserted).
    HdStTextureResourceHandleSharedPtrVector _internalTextureResourceHandles;

    bool _isInitialized : 1;
    bool _hasPtex : 1;
    bool _hasLimitSurfaceEvaluation : 1;
    bool _hasDisplacement : 1;

    TfToken _materialTag;

    HdStMaterialNetwork _networkProcessor;
};

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

inline const TfToken& HdStMaterial::GetMaterialTag() const
{
    return _materialTag;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MATERIAL_H
