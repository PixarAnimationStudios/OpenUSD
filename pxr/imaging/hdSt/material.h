//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIAL_H
#define PXR_IMAGING_HD_ST_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hf/perfLog.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_MaterialNetworkShaderSharedPtr =
        std::shared_ptr<class HdSt_MaterialNetworkShader>;

class HioGlslfx;

class HdStMaterial final: public HdMaterial
{
public:
    HF_MALLOC_TAG_NEW("new HdStMaterial");

    /// For volumes, the corresponding draw items do not use the
    /// HdStShaderCode produced by HdStMaterial. Instead HdStVolume is
    /// using some data from the material to produce its own HdStShaderCode
    /// based on the volume field bindings.
    struct VolumeMaterialData final
    {
        /// glslfx source code for volume
        std::string source;
        HdSt_MaterialParamVector params;
    };

    HDST_API
    HdStMaterial(SdfPath const& id);
    HDST_API
    ~HdStMaterial() override;

    /// Synchronizes state from the delegate to this object.
    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HDST_API
    void Finalize(HdRenderParam *renderParam) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Obtains the GLSLFX code together with supporting information
    /// such as material params and textures to render surfaces.
    HDST_API
    HdSt_MaterialNetworkShaderSharedPtr GetMaterialNetworkShader() const;

    /// Obtains the GLSLFLX code together with material params to
    /// render volumes.
    inline const VolumeMaterialData &GetVolumeMaterialData() const;

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
    void SetMaterialNetworkShader(
        HdSt_MaterialNetworkShaderSharedPtr &shaderCode);

private:
    // Processes the texture descriptors from a material network to
    // create textures using the Storm texture system.
    //
    // Adds buffer specs/sources necessary for textures, e.g., bindless
    // handles or sampling transform for field textures.
    void _ProcessTextureDescriptors(
        HdSceneDelegate * sceneDelegate,
        HdStResourceRegistrySharedPtr const& resourceRegistry,
        std::weak_ptr<HdStShaderCode> const &shaderCode,
        HdStMaterialNetwork::TextureDescriptorVector const &descs,
        HdStShaderCode::NamedTextureHandleVector * texturesFromStorm,
        HdBufferSpecVector * specs,
        HdBufferSourceSharedPtrVector * sources);
    
    bool
    _GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const;

    void _InitFallbackShader();

    static HioGlslfx *_fallbackGlslfx;

    HdSt_MaterialNetworkShaderSharedPtr _materialNetworkShader;
    VolumeMaterialData _volumeMaterialData;

    bool _isInitialized : 1;
    bool _hasPtex : 1;
    bool _hasLimitSurfaceEvaluation : 1;
    bool _hasDisplacement : 1;

    TfToken _materialTag;
    size_t _textureHash;

    HdStMaterialNetwork _networkProcessor;
};

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

inline const HdStMaterial::VolumeMaterialData &
HdStMaterial::GetVolumeMaterialData() const {
    return _volumeMaterialData;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MATERIAL_H
