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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialBufferSourceAndTextureHelper.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
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

#include "pxr/base/tf/staticTokens.h"

#include <boost/pointer_cast.hpp>

// XXX In progress of moving HdStMaterial to use 
// HdSceneDelegate::GetMaterialResource to obtain full material networks
// which will let us receive more general networks in Storm in the future.
#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (limitSurfaceEvaluation)
    (opacity)
);

HioGlslfx *HdStMaterial::_fallbackSurfaceShader = nullptr;

// XXX In progress of deprecating hydra material adapter
static bool _IsEnabledStormMaterialNetworks() {
    static std::string _stormMatNet = 
        TfGetenv("STORM_ENABLE_MATERIAL_NETWORKS");

    return !_stormMatNet.empty() && std::stoi(_stormMatNet) > 0;
}

HdStMaterial::HdStMaterial(SdfPath const &id)
 : HdMaterial(id)
 , _surfaceShader(new HdStSurfaceShader)
 , _isInitialized(false)
 , _hasPtex(false)
 , _hasLimitSurfaceEvaluation(false)
 , _hasDisplacement(false)
 , _materialTag(HdStMaterialTagTokens->defaultMaterialTag)
{
    TF_DEBUG(HDST_MATERIAL_ADDED).Msg("HdStMaterial Created: %s\n",
                                      id.GetText());
}

HdStMaterial::~HdStMaterial()
{
    TF_DEBUG(HDST_MATERIAL_REMOVED).Msg("HdStMaterial Removed: %s\n",
                                        GetId().GetText());
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

    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();
    HdDirtyBits bits = *dirtyBits;

    bool needsRprimMaterialStateUpdate = false;

    std::string fragmentSource;
    std::string geometrySource;
    VtDictionary materialMetadata;
    TfToken materialTag = _materialTag;
    HdMaterialParamVector params;

    if ((bits & DirtyResource) && _IsEnabledStormMaterialNetworks()) {
        // Consume material network
        HdMaterialNetworkMap const& hdNetworkMap = 
            _GetMaterialResource(sceneDelegate);
        HdStMaterialNetwork networkProcessor;
        networkProcessor.ProcessMaterialNetwork(GetId(), hdNetworkMap);
        fragmentSource = networkProcessor.GetFragmentCode();
        geometrySource = networkProcessor.GetGeometryCode();
        materialTag = networkProcessor.GetMaterialTag();
        params = networkProcessor.GetMaterialParams();
    } else {
        // XXX Consume deprecated material
        if (bits & DirtySurfaceShader) {
            fragmentSource = GetSurfaceShaderSource(sceneDelegate);
            geometrySource = GetDisplacementShaderSource(sceneDelegate);
            materialMetadata = GetMaterialMetadata(sceneDelegate);
            materialTag = _GetMaterialTagDeprecated(materialMetadata);
        }
        if (bits & DirtyParams) {
            params = GetMaterialParams(sceneDelegate);
        }
    }

    //
    // Propagate shader changes
    //
    bool shaderIsDirty= ((bits & DirtyResource) || (bits & DirtySurfaceShader));

    if (shaderIsDirty) {
        if (fragmentSource.empty() && geometrySource.empty()) {
            _InitFallbackShader();
            fragmentSource = _fallbackSurfaceShader->GetSurfaceSource();
            geometrySource = _fallbackSurfaceShader->GetGeometrySource();
            materialMetadata = _fallbackSurfaceShader->GetMetadata();
        }

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
        }
    }

    //
    // Mark batches dirty to force batch validation/rebuild.
    //

    if (_isInitialized) {

        // We need to re-batch when the shader or materialTag changes. I.e. when
        // network topology changes or the prim goes from opaque to translucent.
        // We skip this the first time since batches will already be rebuild.

        bool markBatchesDirty = (bits & DirtySurfaceShader) ||
                                (_materialTag != materialTag);

        if (!markBatchesDirty && shaderIsDirty) {
            // XXX cheaper to compare network topology instead fo strings?
            std::string const& oldFragmentSource = 
                _surfaceShader->GetSource(HdShaderTokens->fragmentShader);
            std::string const& oldGeometrySource = 
                _surfaceShader->GetSource(HdShaderTokens->geometryShader);

            markBatchesDirty |= (oldFragmentSource!=fragmentSource) || 
                                (oldGeometrySource!=geometrySource);
        }

        if (markBatchesDirty) {
            sceneDelegate->GetRenderIndex().GetChangeTracker().
                MarkBatchesDirty();
        }
    }

    //
    // Update material parameters
    //
    bool paramsAreDirty = (bits & DirtyResource || bits & DirtyParams);
    if (paramsAreDirty) {
        _surfaceShader->SetParams(params);

        // Release any fallback texture resources
        _fallbackTextureResourceHandles.clear();

        HdSt_MaterialBufferSourceAndTextureHelper sourcesAndTextures;

        bool hasPtex = false;
        for (HdMaterialParam const & param: params) {
            if (param.IsPrimvar()) {
                sourcesAndTextures.ProcessPrimvarMaterialParam(
                    param);
            } else if (param.IsFallback()) {
                if (_IsEnabledStormMaterialNetworks()) {
                    sourcesAndTextures.ProcessFallbackMaterialParam(
                        param, param.fallbackValue);
                } else {
                    // XXX Deprecate.
                    sourcesAndTextures.ProcessFallbackMaterialParam(
                        param, sceneDelegate, GetId());
                }
            } else if (param.IsTexture()) {
                sourcesAndTextures.ProcessTextureMaterialParam(
                    param, 
                    _GetTextureResourceHandle(sceneDelegate, param),
                    &hasPtex);
            }
        }

        _surfaceShader->SetTextureDescriptors(
            sourcesAndTextures.textures);
        _surfaceShader->SetBufferSources(
            sourcesAndTextures.sources, resourceRegistry);

        if (_hasPtex != hasPtex) {
            _hasPtex = hasPtex;
            needsRprimMaterialStateUpdate = true;
        }
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
HdStMaterial::_GetTextureResourceHandle(
        HdSceneDelegate *sceneDelegate,
        HdMaterialParam const &param)
{
    HdStResourceRegistrySharedPtr const& resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdStTextureResourceSharedPtr texResource;
    HdStTextureResourceHandleSharedPtr handle;

    SdfPath const &connection = param.connection;
    if (!connection.IsEmpty()) {
        HdTextureResource::ID texID =
            GetTextureResourceID(sceneDelegate, connection);

        if (texID != HdTextureResource::ID(-1)) {
            // Use render index to convert local texture id into global
            // texture key
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdResourceRegistry::TextureKey texKey =
                                               renderIndex.GetTextureKey(texID);

            HdInstance<HdResourceRegistry::TextureKey,
                        HdTextureResourceSharedPtr> texInstance;

            bool textureResourceFound = false;
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->FindTextureResource
                                  (texKey, &texInstance, &textureResourceFound);

            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and continue onto the
            // next param.
            if (!textureResourceFound) {
                TF_WARN("No texture resource found with path %s",
                    param.connection.GetText());
            } else {
                texResource =
                    boost::dynamic_pointer_cast<HdStTextureResource>
                    (texInstance.GetValue());
            }
        }

        HdResourceRegistry::TextureKey handleKey =
            HdStTextureResourceHandle::GetHandleKey(
                &sceneDelegate->GetRenderIndex(), connection);

        HdInstance<HdResourceRegistry::TextureKey,
                    HdStTextureResourceHandleSharedPtr> handleInstance;

        bool handleFound = false;
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->FindTextureResourceHandle
                              (handleKey, &handleInstance, &handleFound);

        // A bad asset can cause the texture resource to not
        // be found. Hence, issue a warning and continue onto the
        // next param.
        if (!handleFound) {
            TF_WARN("No texture resource handle found with path %s",
                param.connection.GetText());
        } else {
            handle = handleInstance.GetValue();
            handle->SetTextureResource(texResource);
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
        // Fallback texture are only supported for UV textures.
        if (param.textureType != HdTextureType::Uv) {
            return {};
        }
        GlfUVTextureStorageRefPtr texPtr =
            GlfUVTextureStorage::New(1,1, param.fallbackValue);
        GlfTextureHandleRefPtr texture =
            GlfTextureRegistry::GetInstance().GetTextureHandle(texPtr);
        HdStTextureResourceSharedPtr texResource(
            new HdStSimpleTextureResource(texture,
                                          HdTextureType::Uv,
                                          HdWrapClamp,
                                          HdWrapClamp,
                                          HdWrapClamp,
                                          HdMinFilterNearest,
                                          HdMagFilterNearest,
                                          0));
        handle.reset(new HdStTextureResourceHandle(texResource));
        _fallbackTextureResourceHandles.push_back(handle);
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

// XXX Deprecated. This is used for old material descriptions where
// HydraMaterialAdapter calculates the materialTag and we extract it here from
// the metadata. The new '_GetMaterialTag' function is at top of file.
// Once we exclusively use HdMaterialNetwork for storm we can remove this.
TfToken
HdStMaterial::_GetMaterialTagDeprecated(VtDictionary const & metadata) const
{
    VtValue value = TfMapLookupByValue(metadata,
                                       HdShaderTokens->materialTag,
                                       VtValue());

    // A string when the materialTag is hardcoded in the glslfx.
    // A token if the materialTag is auto-determined in MaterialAdapter.
    if (value.IsHolding<TfToken>()) {
        return value.UncheckedGet<TfToken>();
    } else if (value.IsHolding<std::string>()) {
        return TfToken(value.UncheckedGet<std::string>());
    }

    // An empty materialTag on the HdRprimCollection level means: 'ignore all
    // materialTags and add everything to the collection'. Instead we return a
    // default token because we do want materialTags to drive HdSt collections.
    return HdStMaterialTagTokens->defaultMaterialTag;
}

// virtual
HdDirtyBits
HdStMaterial::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}


//virtual
void
HdStMaterial::Reload()
{
    _surfaceShader->Reload();
}

HdStShaderCodeSharedPtr
HdStMaterial::GetShaderCode() const
{
    return boost::static_pointer_cast<HdStShaderCode>(_surfaceShader);
}

void
HdStMaterial::SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode)
{
    _surfaceShader = shaderCode;
}

void
HdStMaterial::_InitFallbackShader()
{
    if (_fallbackSurfaceShader != nullptr) {
        return;
    }

    const TfToken &filePath = HdStPackageFallbackSurfaceShader();

    _fallbackSurfaceShader = new HioGlslfx(filePath);

    // Check fallback shader loaded, if not continue with the invalid shader
    // this would mean the shader compilation fails and the prim would not
    // be drawn.
    TF_VERIFY(_fallbackSurfaceShader->IsValid(),
              "Failed to load fallback surface shader!");
}

HdMaterialNetworkMap const&
HdStMaterial::_GetMaterialResource(HdSceneDelegate* sceneDelegate) const
{
    VtValue vtMat = sceneDelegate->GetMaterialResource(GetId());
    if (vtMat.IsHolding<HdMaterialNetworkMap>()) {
        return vtMat.UncheckedGet<HdMaterialNetworkMap>();
    } else {
        TF_CODING_ERROR("Not a valid material network map");
        static const HdMaterialNetworkMap emptyNetworkMap;
        return emptyNetworkMap;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
