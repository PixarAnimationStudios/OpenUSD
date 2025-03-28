//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H
#define PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/enums.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/base/vt/dictionary.h"

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
#include <MaterialXGenShader/Shader.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;
using HioGlslfxSharedPtr = std::shared_ptr<class HioGlslfx>;
using HdSt_MaterialParamVector = std::vector<class HdSt_MaterialParam>;

/// \class HdStMaterialNetwork
///
/// Helps HdStMaterial process a Hydra material network into shader source code
/// and parameters values.
class HdStMaterialNetwork final
{
public:
    HDST_API
    HdStMaterialNetwork();

    HDST_API
    ~HdStMaterialNetwork();

    /// Process a material network topology and extract all the information we
    /// need from it.
    HDST_API
    void ProcessMaterialNetwork(
        SdfPath const& materialId,
        HdMaterialNetworkMap const& hdNetworkMap,
        HdStResourceRegistry *resourceRegistry);

    HDST_API
    TfToken const& GetMaterialTag() const;

    HDST_API
    std::string const& GetFragmentCode() const;

    HDST_API
    std::string const& GetVolumeCode() const;

    HDST_API
    std::string const& GetDisplacementCode() const;

    HDST_API
    VtDictionary const& GetMetadata() const;

    HDST_API
    HdSt_MaterialParamVector const& GetMaterialParams() const;

    // Information necessary to allocate a texture.
    struct TextureDescriptor
    {
        // Name by which the texture will be accessed, i.e., the name
        // of the accesor for thexture will be HdGet_name(...).
        // It is generated from the input name the corresponding texture
        // node is connected to.
        TfToken name;
        HdStTextureIdentifier textureId;
        HdStTextureType type;
        HdSamplerParameters samplerParameters;
        // Memory request in bytes.
        size_t memoryRequest;

        // The texture is not just identified by a file path attribute
        // on the texture prim but there is special API to texture prim
        // to obtain the texture.
        //
        // This is used for draw targets.
        bool useTexturePrimToFindTexture;
        // This is used for draw targets and hashing.
        SdfPath texturePrim;
    };

    using TextureDescriptorVector = std::vector<TextureDescriptor>;

    HDST_API
    TextureDescriptorVector const& GetTextureDescriptors() const;

private:
    TfToken _materialTag;
    std::string _fragmentSource;
    std::string _volumeSource;
    std::string _displacementSource;
    VtDictionary _materialMetadata;
    HdSt_MaterialParamVector _materialParams;
    TextureDescriptorVector _textureDescriptors;
    HioGlslfxSharedPtr _surfaceGfx;
    size_t _surfaceGfxHash;
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    MaterialX::ShaderPtr _materialXGfx;
#endif
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
