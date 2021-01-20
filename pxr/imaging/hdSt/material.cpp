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
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialBufferSourceAndTextureHelper.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/uvTextureStorage.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (limitSurfaceEvaluation)
    (opacity)
);

HioGlslfx *HdStMaterial::_fallbackGlslfx = nullptr;


HdStMaterial::HdStMaterial(SdfPath const &id)
 : HdMaterial(id)
 , _surfaceShader(new HdStSurfaceShader)
 , _isInitialized(false)
 , _hasPtex(false)
 , _hasLimitSurfaceEvaluation(false)
 , _hasDisplacement(false)
 , _materialTag(HdStMaterialTagTokens->defaultMaterialTag)
 , _textureHash(0)
{
    TF_DEBUG(HDST_MATERIAL_ADDED).Msg("HdStMaterial Created: %s\n",
                                      id.GetText());
}

HdStMaterial::~HdStMaterial()
{
    TF_DEBUG(HDST_MATERIAL_REMOVED).Msg("HdStMaterial Removed: %s\n",
                                        GetId().GetText());
}

// Check whether the texture node points to a render buffer and
// use information from it to get the texture identifier.
//
// Return false if the deprecated HdSceneDelegate::GetTextureResource
// should be used.
//
static
bool
_GetTextureIdentifier(
    HdStMaterialNetwork::TextureDescriptor const &desc,
    HdSceneDelegate * const sceneDelegate,
    HdStTextureIdentifier * const textureId)
{
    if (!desc.useTexturePrimToFindTexture) {
        *textureId = desc.textureId;
        return true;
    }

    // Get render buffer texture node is pointing to.
    if (HdStRenderBuffer * const renderBuffer =
            dynamic_cast<HdStRenderBuffer*>(
                sceneDelegate->GetRenderIndex().GetBprim(
                    HdPrimTypeTokens->renderBuffer, desc.texturePrim))) {

        if (desc.type != HdTextureType::Uv) {
            TF_CODING_ERROR(
                "Non-UV texture type in descriptor using render buffer.");
            return false;
        }

        *textureId = renderBuffer->GetTextureIdentifier(
            /* multiSampled = */ false);
        return true;
    }

    return false;
}

static size_t
_GetTextureDescriptorHash(
    bool useScenePath,
    HdStMaterialNetwork::TextureDescriptor const& desc,
    HdStTextureIdentifier const& textureId)
{
    if (useScenePath) {
        return hash_value(desc.texturePrim);
    } else {
        return TfHash::Combine(
            textureId,
            desc.samplerParameters.wrapS,
            desc.samplerParameters.wrapT,
            desc.samplerParameters.wrapR,
            desc.samplerParameters.minFilter,
            desc.samplerParameters.magFilter);
    }
}

void
HdStMaterial::_ProcessTextureDescriptors(
    HdSceneDelegate * const sceneDelegate,
    HdStResourceRegistrySharedPtr const& resourceRegistry,
    std::weak_ptr<HdStShaderCode> const &shaderCode,
    HdStMaterialNetwork::TextureDescriptorVector const &descs,
    HdStShaderCode::NamedTextureHandleVector * const texturesFromStorm,
    HdStShaderCode::TextureDescriptorVector * const texturesFromSceneDelegate,
    HdBufferSpecVector * const specs,
    HdBufferSourceSharedPtrVector * const sources)
{
    const bool bindlessTextureEnabled
        = GlfContextCaps::GetInstance().bindlessTextureEnabled;

    for (HdStMaterialNetwork::TextureDescriptor const &desc : descs) {
        HdStTextureIdentifier texId;
        if (_GetTextureIdentifier(desc, sceneDelegate, &texId)) {
            HdStTextureHandleSharedPtr const textureHandle = 
                resourceRegistry->AllocateTextureHandle(
                    texId,
                    desc.type,
                    desc.samplerParameters,
                    desc.memoryRequest,
                    bindlessTextureEnabled,
                    shaderCode);

            // Note about batching hashes:
            // If this is our first sync, try to hash using the asset path.
            // If we're on our 2nd+ sync, just use the texture prim path.
            //
            // This will aggressively batch textured prims together as long as
            // they are 100% static; if they are dynamic, we assume that the
            // textures are dynamic, and we split the batches of textured prims.
            //
            // This tries to balance two competing concerns:
            // 1.) Static textured simple geometry (like billboard placeholders)
            //     really need to be batched together; otherwise, the render
            //     cost is dominated by the switching cost between batches.
            // 2.) Objects with animated textures will change their texture hash
            //     every frame.  If the hash is based on asset path, we need to
            //     rebuild batches every frame, which is untenable. If it's
            //     based on scene graph path (meaning split into its own batch),
            //     we don't need to update batching.
            //
            // Better batch invalidation (i.e., not global) would really help
            // us here, as would hints from the scene about how likely the
            // textures are to change.  Maybe in the future...

            texturesFromStorm->push_back(
                { desc.name,
                  desc.type,
                  textureHandle,
                  _GetTextureDescriptorHash(_isInitialized, desc, texId)});
        } else {
            // If above fails, fallback to the old-style
            // HdSceneDelegat::GetTextureResource which is deprecated.
            HdStTextureResourceHandleSharedPtr const textureResource =
                _GetTextureResourceHandleFromSceneDelegate(
                    sceneDelegate,
                    resourceRegistry,
                    desc);
            
            HdSt_MaterialBufferSourceAndTextureHelper::
                ProcessTextureMaterialParam(
                    desc.name, desc.texturePrim,
                    textureResource,
                    specs, sources, texturesFromSceneDelegate);
        }
    }

    HdSt_TextureBinder::GetBufferSpecs(
        *texturesFromStorm, bindlessTextureEnabled, specs);
}

/* virtual */
void
HdStMaterial::Sync(HdSceneDelegate *sceneDelegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    HdStResourceRegistrySharedPtr const& resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdDirtyBits bits = *dirtyBits;

    if (!(bits & DirtyResource) && !(bits & DirtyParams)) {
        *dirtyBits = Clean;
        return;
    }

    bool needsRprimMaterialStateUpdate = false;
    bool markBatchesDirty = false;

    std::string fragmentSource;
    std::string geometrySource;
    VtDictionary materialMetadata;
    TfToken materialTag = _materialTag;
    HdSt_MaterialParamVector params;
    HdStMaterialNetwork::TextureDescriptorVector textureDescriptors;

    VtValue vtMat = sceneDelegate->GetMaterialResource(GetId());
    if (vtMat.IsHolding<HdMaterialNetworkMap>()) {
        HdMaterialNetworkMap const& hdNetworkMap =
            vtMat.UncheckedGet<HdMaterialNetworkMap>();
        if (!hdNetworkMap.terminals.empty() && !hdNetworkMap.map.empty()) {
            _networkProcessor.ProcessMaterialNetwork(GetId(), hdNetworkMap,
                                                    resourceRegistry.get());
            fragmentSource = _networkProcessor.GetFragmentCode();
            geometrySource = _networkProcessor.GetGeometryCode();
            materialMetadata = _networkProcessor.GetMetadata();
            materialTag = _networkProcessor.GetMaterialTag();
            params = _networkProcessor.GetMaterialParams();
                textureDescriptors = _networkProcessor.GetTextureDescriptors();
            }
        }

    if (fragmentSource.empty() && geometrySource.empty()) {
        _InitFallbackShader();
        fragmentSource = _fallbackGlslfx->GetSurfaceSource();
        // Note that we don't want displacement on purpose for the 
        // fallback material.
        geometrySource = std::string();
        materialMetadata = _fallbackGlslfx->GetMetadata();
    }

    // If we're updating the fragment or geometry source, we need to rebatch
    // anything that uses this material.
    std::string const& oldFragmentSource = 
        _surfaceShader->GetSource(HdShaderTokens->fragmentShader);
    std::string const& oldGeometrySource = 
        _surfaceShader->GetSource(HdShaderTokens->geometryShader);

    markBatchesDirty |= (oldFragmentSource!=fragmentSource) || 
                        (oldGeometrySource!=geometrySource);

    _surfaceShader->SetFragmentSource(fragmentSource);
    _surfaceShader->SetGeometrySource(geometrySource);

    bool hasDisplacement = !(geometrySource.empty());

    if (_hasDisplacement != hasDisplacement) {
        _hasDisplacement = hasDisplacement;
        needsRprimMaterialStateUpdate = true;
    }

    bool hasLimitSurfaceEvaluation =
        _GetHasLimitSurfaceEvaluation(materialMetadata);

    if (_hasLimitSurfaceEvaluation != hasLimitSurfaceEvaluation) {
        _hasLimitSurfaceEvaluation = hasLimitSurfaceEvaluation;
        needsRprimMaterialStateUpdate = true;
    }

    if (_materialTag != materialTag) {
        _materialTag = materialTag;
        _surfaceShader->SetMaterialTag(_materialTag);
        needsRprimMaterialStateUpdate = true;

        // If the material tag changes, we'll need to rebatch.
        markBatchesDirty = true;
    }

    _surfaceShader->SetEnabledPrimvarFiltering(true);

    //
    // Update material parameters
    //
    _surfaceShader->SetParams(params);

    // Release any fallback texture resources
    _internalTextureResourceHandles.clear();

    HdBufferSpecVector specs;
    HdBufferSourceSharedPtrVector sources;

    bool hasPtex = false;
    for (HdSt_MaterialParam const & param: params) {
        if (param.IsPrimvarRedirect() || param.IsFallback() || 
            param.IsTransform2d()) {
            HdStSurfaceShader::AddFallbackValueToSpecsAndSources(
                param, &specs, &sources);
        } else if (param.IsTexture()) {
            // Fallback value only supported for Uv and Field textures.
            if (param.textureType == HdTextureType::Uv ||
                param.textureType == HdTextureType::Field) {
                HdStSurfaceShader::AddFallbackValueToSpecsAndSources(
                    param, &specs, &sources);
            }
            if (param.textureType == HdTextureType::Ptex) {
                hasPtex = true;
            }
        }
    }

    // Textures from scene delegate have a different type.
    HdStShaderCode::TextureDescriptorVector textureResourceDescriptors;

    // Textures created using Storm texture system.
    HdStShaderCode::NamedTextureHandleVector textures;
    
    _ProcessTextureDescriptors(
        sceneDelegate,
        resourceRegistry,
                _surfaceShader,
        textureDescriptors,
        &textures,
        &textureResourceDescriptors,
        &specs,
        &sources);

    // Check if the texture hash has changed; if so, we need to ask for a
    // re-batch.  We only look at NamedTextureHandles because the legacy system
    // hashes based on scene graph path, meaning each textured gprim gets its
    // own batch.
    size_t textureHash = 0;
    for (HdStShaderCode::NamedTextureHandle const& tex : textures) {
        textureHash = TfHash::Combine(textureHash, tex.hash);
    }

    if (_textureHash != textureHash) {
        _textureHash = textureHash;
        markBatchesDirty = true;
    }

    _surfaceShader->SetNamedTextureHandles(textures);
    _surfaceShader->SetTextureDescriptors(textureResourceDescriptors);    
    _surfaceShader->SetBufferSources(
        specs, std::move(sources), resourceRegistry);

    if (_hasPtex != hasPtex) {
        _hasPtex = hasPtex;
        needsRprimMaterialStateUpdate = true;
    }

    if (markBatchesDirty && _isInitialized) {
        // Only invalidate batches if this isn't our first round through sync.
        // If this is the initial sync, we haven't formed batches yet.
        sceneDelegate->GetRenderIndex().GetChangeTracker().MarkBatchesDirty();
    }

    if (needsRprimMaterialStateUpdate && _isInitialized) {
        // XXX Forcing rprims to have a dirty material id to re-evaluate
        // their material state as we don't know which rprims are bound to
        // this one. We can skip this invalidation the first time this
        // material is Sync'd since any affected Rprim should already be
        // marked with a dirty material id.
        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();
        changeTracker.MarkAllRprimsDirty(HdChangeTracker::DirtyMaterialId);
    }

    _isInitialized = true;
    *dirtyBits = Clean;
}

HdStTextureResourceHandleSharedPtr
HdStMaterial::_GetTextureResourceHandleFromSceneDelegate(
        HdSceneDelegate * const sceneDelegate,
        HdStResourceRegistrySharedPtr const& resourceRegistry,
        HdStMaterialNetwork::TextureDescriptor const &desc)
{
    HdStTextureResourceSharedPtr texResource;
    HdStTextureResourceHandleSharedPtr handle;

    SdfPath const &connection = desc.texturePrim;

    if (!connection.IsEmpty()) {
        HdTextureResource::ID texID =
            GetTextureResourceID(sceneDelegate, connection);

        // Step 1.
        // Try to locate the texture in resource registry.
        // A Bprim might have been inserted for this texture.
        //
        if (texID != HdTextureResource::ID(-1)) {
            // Use render index to convert local texture id into global
            // texture key
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdResourceRegistry::TextureKey texKey =
                                               renderIndex.GetTextureKey(texID);

            bool textureResourceFound = false;
            HdInstance<HdStTextureResourceSharedPtr> texInstance =
                resourceRegistry->FindTextureResource
                                  (texKey, &textureResourceFound);

            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and continue onto the
            // next param.
            if (!textureResourceFound) {
                TF_WARN("No texture resource found with path %s",
                    connection.GetText());
            } else {
                texResource = texInstance.GetValue();
            }
        }

        HdResourceRegistry::TextureKey handleKey =
            HdStTextureResourceHandle::GetHandleKey(
                &sceneDelegate->GetRenderIndex(), connection);

        bool handleFound = false;
        HdInstance<HdStTextureResourceHandleSharedPtr> handleInstance =
            resourceRegistry->FindTextureResourceHandle
                              (handleKey, &handleFound);

        if (handleFound) {
            handle = handleInstance.GetValue();
            handle->SetTextureResource(texResource);
        }

        // Step 2.
        // If no texture was found in the registry, it might be a texture we
        // discovered in the material network. If we can load it we will store
        // the handle internally in this material.
        //
        if (!texResource) {
            HdTextureResourceSharedPtr hdTexResource = 
                sceneDelegate->GetTextureResource(connection);
            texResource = std::static_pointer_cast<HdStTextureResource>(
                hdTexResource);
        }
    }

    // There are many reasons why texResource could be null here:
    // - A missing or invalid connection path,
    // - A deliberate (-1) or accidental invalid texture id
    // - Scene delegate failed to return a texture resource (due to asset error)
    //
    // In all these cases fallback to a simple texture with the provided
    // fallback value
    //
    // XXX todo handle fallback Ptex textures
    if (!(handle && handle->GetTextureResource())) {

        if (!texResource) {
            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and insert a fallback texture.
            if (!connection.IsEmpty()) {
                TF_WARN("Texture not found. Using fallback texture for: %s",
                        connection.GetText());
            }

            // Fallback texture are only supported for UV textures.
            if (desc.type != HdTextureType::Uv) {
                return {};
            }
            GlfUVTextureStorageRefPtr texPtr =
                GlfUVTextureStorage::New(1,1, desc.fallbackValue);
            GlfTextureHandleRefPtr texture =
                GlfTextureRegistry::GetInstance().GetTextureHandle(texPtr);

            texResource = HdStTextureResourceSharedPtr(
                new HdStSimpleTextureResource(texture,
                                              HdTextureType::Uv,
                                              HdWrapClamp,
                                              HdWrapClamp,
                                              HdWrapClamp,
                                              HdMinFilterNearest,
                                              HdMagFilterNearest,
                                              0));
        }

        if (texResource) {
            handle.reset(new HdStTextureResourceHandle(texResource));
            _internalTextureResourceHandles.push_back(handle);
        }
    }

    return handle;
}

bool
HdStMaterial::_GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const
{
    VtValue value = TfMapLookupByValue(metadata,
                                       _tokens->limitSurfaceEvaluation,
                                       VtValue());
    return value.IsHolding<bool>() && value.Get<bool>();
}

// virtual
HdDirtyBits
HdStMaterial::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

HdStShaderCodeSharedPtr
HdStMaterial::GetShaderCode() const
{
    return _surfaceShader;
}

void
HdStMaterial::SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode)
{
    _surfaceShader = shaderCode;
}

void
HdStMaterial::_InitFallbackShader()
{
    if (_fallbackGlslfx != nullptr) {
        return;
    }

    const TfToken &filePath = HdStPackageFallbackSurfaceShader();

    _fallbackGlslfx = new HioGlslfx(filePath);

    // Check fallback shader loaded, if not continue with the invalid shader
    // this would mean the shader compilation fails and the prim would not
    // be drawn.
    TF_VERIFY(_fallbackGlslfx->IsValid(),
              "Failed to load fallback surface shader!");
}

PXR_NAMESPACE_CLOSE_SCOPE
