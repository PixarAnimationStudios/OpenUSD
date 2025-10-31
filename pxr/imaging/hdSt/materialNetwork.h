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

class SdrRegistry;
class HdStResourceRegistry;
using HioGlslfxSharedPtr = std::shared_ptr<class HioGlslfx>;
using HdSt_MaterialParamVector = std::vector<class HdSt_MaterialParam>;
struct HdSt_MaterialFilterTask;
using HdSt_MaterialFilterTaskSharedPtr =
    std::shared_ptr<HdSt_MaterialFilterTask>;

extern HdMaterialNode2 const*
HdSt_GetTerminalNode(
    HdMaterialNetwork2 const& network,
    TfToken const& terminalName,
    SdfPath * terminalNodePath);

/// Encapsulates the input data for MaterialX codegen as well as metadata
/// necessary for completing `HdStMaterial::Sync` based on the result of the
/// codegen.
/// This object can either live on the stack, if MaterialX codegen happens
/// synchronously, or on the heap, owned by the respective Sprim, if the
/// codegen happens in parallel tasks.
///
struct ARCH_EXPORT_TYPE HdSt_MaterialFilterTask final
{
    HdMaterialNetwork2 hdNetwork;
    HdMaterialNode2 const* terminalNode = nullptr;  // pointer to a node
                                                    // in the above network
    SdfPath terminalNodePath;   // path to the above node

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    // Stores the mappings between the node paths in the original
    // HdMaterialNetwork to the corresponding anonymized node paths
    using OrigToAnonSdfPathMap =
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;
    OrigToAnonSdfPathMap origToAnonSdfPathMap;

    /// Build the `anonNetwork`, equivalent to the given hdNetwork but anonymized
    /// and stripped of non-topological parameters to better re-use the generated
    /// shader.
    /// Returns the hash of the anonymized network.
    size_t BuildAnonymizedMaterialNetwork(
        HdMaterialNetwork2* anonNetwork);

    void AddFallbackDomeLightTextureNode();

    void AddMaterialXParams(
        MaterialX::Shader const& mxShader,
        HdSt_MaterialParamVector* materialParams);

    bool IsMaterialX(SdrRegistry* sdrRegistry) const;
#endif
};

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

    /// Process the necessary network information cached in the filter task.
    HDST_API
    void ProcessFilterTask(
        SdfPath const& materialId,
        HdSt_MaterialFilterTaskSharedPtr filterTask,
        bool isVolume,
        HdStResourceRegistry *resourceRegistry);

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
        // of the accessor for the texture will be HdGet_name(...).
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
