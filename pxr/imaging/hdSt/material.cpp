//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/materialParam.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hio/glslfx.h"

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
 , _materialNetworkShader(std::make_shared<HdSt_MaterialNetworkShader>())
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
static
HdStTextureIdentifier
_GetTextureIdentifier(
    HdStMaterialNetwork::TextureDescriptor const &desc,
    HdSceneDelegate * const sceneDelegate)
{
    if (!desc.useTexturePrimToFindTexture) {
        return desc.textureId;
    }

    // Get render buffer texture node is pointing to.
    if (HdStRenderBuffer * const renderBuffer =
            dynamic_cast<HdStRenderBuffer*>(
                sceneDelegate->GetRenderIndex().GetBprim(
                    HdPrimTypeTokens->renderBuffer, desc.texturePrim))) {

        if (desc.type == HdStTextureType::Uv) {
            return renderBuffer->GetTextureIdentifier(
                /* multiSampled = */ false);
        }

        TF_CODING_ERROR(
            "Non-UV texture type in descriptor using render buffer.");
    }

    return HdStTextureIdentifier();
}

static size_t
_GetTextureHandleHash(
    HdStTextureHandleSharedPtr const& textureHandle)
{
    const HdSamplerParameters &samplerParams =
        textureHandle->GetSamplerParameters();

    return TfHash::Combine(
        textureHandle->GetTextureObject()->GetTextureIdentifier(),
        samplerParams.wrapS,
        samplerParams.wrapT,
        samplerParams.wrapR,
        samplerParams.minFilter,
        samplerParams.magFilter,
        samplerParams.borderColor,
        samplerParams.enableCompare,
        samplerParams.compareFunction,
        samplerParams.maxAnisotropy);
}

void
HdStMaterial::_ProcessTextureDescriptors(
    HdSceneDelegate * const sceneDelegate,
    HdStResourceRegistrySharedPtr const& resourceRegistry,
    std::weak_ptr<HdStShaderCode> const &shaderCode,
    HdStMaterialNetwork::TextureDescriptorVector const &descs,
    HdStShaderCode::NamedTextureHandleVector * const texturesFromStorm,
    HdBufferSpecVector * const specs,
    HdBufferSourceSharedPtrVector * const sources)
{
    for (HdStMaterialNetwork::TextureDescriptor const &desc : descs) {
        HdStTextureHandleSharedPtr const textureHandle = 
            resourceRegistry->AllocateTextureHandle(
                _GetTextureIdentifier(desc, sceneDelegate),
                desc.type,
                desc.samplerParameters,
                desc.memoryRequest,
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
              _isInitialized
                  ? hash_value(desc.texturePrim)
                  : _GetTextureHandleHash(textureHandle) });
    }

    bool const doublesSupported = resourceRegistry->GetHgi()->
        GetCapabilities()->IsSet(
            HgiDeviceCapabilitiesBitsShaderDoublePrecision);
    HdSt_TextureBinder::GetBufferSpecs(*texturesFromStorm, specs,
                                       doublesSupported);
}

/* virtual */
void
HdStMaterial::Sync(HdSceneDelegate *sceneDelegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

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
    std::string displacementSource;
    std::string volumeSource;
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
            volumeSource = _networkProcessor.GetVolumeCode();
            displacementSource = _networkProcessor.GetDisplacementCode();
            materialMetadata = _networkProcessor.GetMetadata();
            materialTag = _networkProcessor.GetMaterialTag();
            params = _networkProcessor.GetMaterialParams();
                textureDescriptors = _networkProcessor.GetTextureDescriptors();
        }
    }

    // Use fallback shader when there is no source for
    // fragment and displacement shader.
    if (fragmentSource.empty() &&
        displacementSource.empty()) {

        _InitFallbackShader();
        fragmentSource = _fallbackGlslfx->GetSurfaceSource();
        materialMetadata = _fallbackGlslfx->GetMetadata();
    }

    // Update volume material data.
    if (_volumeMaterialData.source != volumeSource) {
        // If we're updating the volume source, we need to rebatch anything that
        // uses this material.
        markBatchesDirty = true;
        _volumeMaterialData.source = std::move(volumeSource);

        if (_volumeMaterialData.source.empty()) {
            // Material does not provide a volume shader.
            // In this case, we leave it to _ComputeVolumeMaterialData in
            // volume.cpp to use the fallback volume data.
            // The params of this material are also irrelevant for volumes in
            // this case.
            _volumeMaterialData.params = {};
        } else {
            _volumeMaterialData.params = params;
        }
    }

    // If we're updating the fragment or displacement source, we need to
    // rebatch anything that uses this material.
    std::string const& oldFragmentSource = 
        _materialNetworkShader->GetSource(HdShaderTokens->fragmentShader);
    std::string const& oldDisplacementSource =
        _materialNetworkShader->GetSource(HdShaderTokens->displacementShader);

    markBatchesDirty |= (oldFragmentSource!=fragmentSource) || 
                        (oldDisplacementSource!=displacementSource);

    _materialNetworkShader->SetFragmentSource(fragmentSource);
    _materialNetworkShader->SetDisplacementSource(displacementSource);

    bool hasDisplacement = !(displacementSource.empty());

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
        _materialNetworkShader->SetMaterialTag(_materialTag);
        needsRprimMaterialStateUpdate = true;

        // If the material tag changes, we'll need to rebatch.
        markBatchesDirty = true;
    }

    _materialNetworkShader->SetEnabledPrimvarFiltering(true);

    //
    // Update material parameters
    //
    _materialNetworkShader->SetParams(params);

    HdBufferSpecVector specs;
    HdBufferSourceSharedPtrVector sources;

    bool hasPtex = false;
    for (HdSt_MaterialParam const & param: params) {
        if (param.IsPrimvarRedirect() || param.IsFallback() || 
            param.IsTransform2d()) {
            HdSt_MaterialNetworkShader::AddFallbackValueToSpecsAndSources(
                param, &specs, &sources);
        } else if (param.IsTexture()) {
            HdSt_MaterialNetworkShader::AddFallbackValueToSpecsAndSources(
                param, &specs, &sources);
            if (param.textureType == HdStTextureType::Ptex) {
                hasPtex = true;
            }
        }
    }

    // Textures created using Storm texture system.
    HdStShaderCode::NamedTextureHandleVector textures;
    
    _ProcessTextureDescriptors(
        sceneDelegate,
        resourceRegistry,
        _materialNetworkShader,
        textureDescriptors,
        &textures,
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

    _materialNetworkShader->SetNamedTextureHandles(textures);
    _materialNetworkShader->SetBufferSources(
        specs, std::move(sources), resourceRegistry);

    if (_hasPtex != hasPtex) {
        _hasPtex = hasPtex;
        needsRprimMaterialStateUpdate = true;
    }

    if (markBatchesDirty && _isInitialized) {
        // Only invalidate batches if this isn't our first round through sync.
        // If this is the initial sync, we haven't formed batches yet.
        HdStMarkDrawBatchesDirty(renderParam);
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

/*virtual*/
void
HdStMaterial::Finalize(HdRenderParam *renderParam)
{
    // Flag GC to reclaim resources owned by the surface shader.
    HdStMarkGarbageCollectionNeeded(renderParam);
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

HdSt_MaterialNetworkShaderSharedPtr
HdStMaterial::GetMaterialNetworkShader() const
{
    return _materialNetworkShader;
}

void
HdStMaterial::SetMaterialNetworkShader(
        HdSt_MaterialNetworkShaderSharedPtr &materialNetworkShader)
{
    _materialNetworkShader = materialNetworkShader;
}

void
HdStMaterial::_InitFallbackShader()
{
    if (_fallbackGlslfx != nullptr) {
        return;
    }

    const TfToken &filePath = HdStPackageFallbackMaterialNetworkShader();

    _fallbackGlslfx = new HioGlslfx(filePath);

    // Check fallback shader loaded, if not continue with the invalid shader
    // this would mean the shader compilation fails and the prim would not
    // be drawn.
    TF_VERIFY(_fallbackGlslfx->IsValid(),
              "Failed to load fallback surface shader!");
}

PXR_NAMESPACE_CLOSE_SCOPE
