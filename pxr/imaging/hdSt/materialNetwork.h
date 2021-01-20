//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H
#define PXR_IMAGING_HD_ST_MATERIAL_NETWORK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

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
    std::string const& GetGeometryCode() const;

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
        HdTextureType type;
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
    std::string _geometrySource;
    VtDictionary _materialMetadata;
    HdSt_MaterialParamVector _materialParams;
    TextureDescriptorVector _textureDescriptors;
    HioGlslfxSharedPtr _surfaceGfx;
    size_t _surfaceGfxHash;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
