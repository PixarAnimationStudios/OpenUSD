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
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/domeLightComputations.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (domeLightIrradiance)
    (domeLightPrefilter) 
    (domeLightBRDF)
);


HdStSimpleLightingShader::HdStSimpleLightingShader() 
    : _lightingContext(GlfSimpleLightingContext::New())
    , _bindingMap(TfCreateRefPtr(new GlfBindingMap()))
    , _useLighting(true)
    , _glslfx(std::make_unique<HioGlslfx>(HdStPackageSimpleLightingShader()))
{
}

HdStSimpleLightingShader::~HdStSimpleLightingShader() = default;

/* virtual */
HdStSimpleLightingShader::ID
HdStSimpleLightingShader::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    const TfToken glslfxFile = HdStPackageSimpleLightingShader();
    const size_t numLights =
        _useLighting ? _lightingContext->GetNumLightsUsed() : 0;
    const bool useShadows =
        _useLighting ? _lightingContext->GetUseShadows() : false;
    const size_t numShadows =
        useShadows ? _lightingContext->ComputeNumShadowsUsed() : 0;

    size_t hash = glslfxFile.Hash();
    boost::hash_combine(hash, numLights);
    boost::hash_combine(hash, useShadows);
    boost::hash_combine(hash, numShadows);
    boost::hash_combine(hash, _lightingContext->ComputeShaderSourceHash());

    return (ID)hash;
}

/* virtual */
std::string
HdStSimpleLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    const std::string source = _glslfx->GetSource(shaderStageKey);

    if (source.empty()) return source;

    std::stringstream defineStream;
    const size_t numLights =
        _useLighting ? _lightingContext->GetNumLightsUsed() : 0;
    const bool useShadows =
        _useLighting ? _lightingContext->GetUseShadows() : false;
    const size_t numShadows =
        useShadows ? _lightingContext->ComputeNumShadowsUsed() : 0;
    defineStream << "#define NUM_LIGHTS " << numLights<< "\n";
    defineStream << "#define USE_SHADOWS " << (int)(useShadows) << "\n";
    defineStream << "#define NUM_SHADOWS " << numShadows << "\n";
    if (useShadows) {
        const bool useBindlessShadowMaps =
            GlfSimpleShadowArray::GetBindlessShadowMapsEnabled();;
        defineStream << "#define USE_BINDLESS_SHADOW_TEXTURES "
                     << int(useBindlessShadowMaps) << "\n";
    }

    const std::string postSurfaceShader =
        _lightingContext->ComputeShaderSource(shaderStageKey);

    if (!postSurfaceShader.empty()) {
        defineStream << "#define HD_HAS_postSurfaceShader\n";
    }

    return defineStream.str() + postSurfaceShader + source;
}

/* virtual */
void
HdStSimpleLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                    GfMatrix4d const &projectionMatrix)
{
    _lightingContext->SetCamera(worldToViewMatrix, projectionMatrix);
}

static
bool
_HasDomeLight(GlfSimpleLightingContextRefPtr const &ctx)
{
    for (auto const& light : ctx->GetLights()){
        if (light.IsDomeLight()) {
            return true;
        }
    }
    return false;
}

/* virtual */
void
HdStSimpleLightingShader::BindResources(const int program,
                                        HdSt_ResourceBinder const &binder,
                                        HdRenderPassState const &state)
{
    // XXX: we'd like to use HdSt_ResourceBinder instead of GlfBindingMap.
    _bindingMap->ResetUniformBindings(
                binder.GetNumReservedUniformBlockLocations());
    _lightingContext->InitUniformBlockBindings(_bindingMap);
    _bindingMap->AssignUniformBindingsToProgram(program);
    _lightingContext->BindUniformBlocks(_bindingMap);

    _bindingMap->ResetSamplerBindings(
                binder.GetNumReservedTextureUnits());
    _lightingContext->InitSamplerUnitBindings(_bindingMap);
    _bindingMap->AssignSamplerUnitsToProgram(program);
    _lightingContext->BindSamplers(_bindingMap);

    HdSt_TextureBinder::BindResources(binder, false, _namedTextureHandles);

    binder.BindShaderResources(this);
}

/* virtual */
void
HdStSimpleLightingShader::UnbindResources(const int program,
                                          HdSt_ResourceBinder const &binder,
                                          HdRenderPassState const &state)
{
    // XXX: we'd like to use HdSt_ResourceBinder instead of GlfBindingMap.
    //
    _lightingContext->UnbindSamplers(_bindingMap);

    HdSt_TextureBinder::UnbindResources(binder, false, _namedTextureHandles);
}

/*virtual*/
void
HdStSimpleLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // For now we assume that the only simple light with a texture is
    // a domeLight (ignoring RectLights, and multiple domeLights)

    _lightTextureParams.clear();
    if(_HasDomeLight(_lightingContext)) {
        // irradiance map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightIrradiance,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdTextureType::Uv));
        // prefilter map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightPrefilter,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdTextureType::Uv));
        // BRDF texture
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightBRDF,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdTextureType::Uv));
    }
}

HdSt_MaterialParamVector const& 
HdStSimpleLightingShader::GetParams() const 
{
    return _lightTextureParams;
}

void
HdStSimpleLightingShader::SetLightingStateFromOpenGL()
{
    _lightingContext->SetStateFromOpenGL();
}

void
HdStSimpleLightingShader::SetLightingState(
    GlfSimpleLightingContextPtr const &src)
{
    if (src) {
        _useLighting = true;
        _lightingContext->SetUseLighting(!src->GetLights().empty());
        _lightingContext->SetLights(src->GetLights());
        _lightingContext->SetMaterial(src->GetMaterial());
        _lightingContext->SetSceneAmbient(src->GetSceneAmbient());
        _lightingContext->SetShadows(src->GetShadows());
    } else {
        // XXX:
        // if src is null, turn off lights (this is temporary used for shadowmap drawing).
        // see GprimUsdBaseIcBatch::Draw()
        _useLighting = false;
    }
}

static
const std::string &
_GetResolvedDomeLightEnvironmentFilePath(
    const GlfSimpleLightingContextRefPtr &ctx)
{
    static const std::string empty;

    if (!ctx) {
        return empty;
    }
    
    const GlfSimpleLightVector & lights = ctx->GetLights();
    for (auto it = lights.rbegin(); it != lights.rend(); ++it) {
        if (it->IsDomeLight()) {
            return it->GetDomeLightTextureFile().GetResolvedPath();
        }
    }

    return empty;
}

const HdStTextureHandleSharedPtr &
HdStSimpleLightingShader::GetTextureHandle(const TfToken &name) const
{
    for (auto const & namedTextureHandle : _namedTextureHandles) {
        if (namedTextureHandle.name == name) {
            return namedTextureHandle.handle;
        }
    }

    static const HdStTextureHandleSharedPtr empty;
    return empty;
}

static
HdStShaderCode::NamedTextureHandle
_MakeNamedTextureHandle(
    const TfToken &name,
    const std::string &texturePath,
    const HdWrap wrapModeS,
    const HdWrap wrapModeT,
    const HdWrap wrapModeR,
    const HdMinFilter minFilter,
    HdStResourceRegistry * const resourceRegistry,
    HdStShaderCodeSharedPtr const &shader)
{
    const HdStTextureIdentifier textureId(
        TfToken(texturePath + "[" + name.GetString() + "]"),
        std::make_unique<HdStDynamicUvSubtextureIdentifier>());

    const HdSamplerParameters samplerParameters{
        wrapModeS, wrapModeT, wrapModeR,
        minFilter, HdMagFilterLinear};

    HdStTextureHandleSharedPtr const textureHandle =
        resourceRegistry->AllocateTextureHandle(
            textureId,
            HdTextureType::Uv,
            samplerParameters,
            /* memoryRequest = */ 0,
            /* createBindlessHandle = */ false,
            shader);

    return { name,
             HdTextureType::Uv,
             textureHandle,
             name.Hash() };
}

void
HdStSimpleLightingShader::AllocateTextureHandles(HdSceneDelegate *const delegate)
{
    const std::string &resolvedPath =
        _GetResolvedDomeLightEnvironmentFilePath(_lightingContext);
    if (resolvedPath.empty()) {
        _domeLightEnvironmentTextureHandle = nullptr;
        _namedTextureHandles.clear();
        return;
    }

    if (_domeLightEnvironmentTextureHandle) {
        HdStTextureObjectSharedPtr const &textureObject =
            _domeLightEnvironmentTextureHandle->GetTextureObject();
        HdStTextureIdentifier const &textureId =
            textureObject->GetTextureIdentifier();
        if (textureId.GetFilePath() == resolvedPath) {
            // Same environment map, no need to recompute
            // dome light textures.
            return;
        }
    }

    HdStResourceRegistry * const resourceRegistry =
        dynamic_cast<HdStResourceRegistry*>(
            delegate->GetRenderIndex().GetResourceRegistry().get());
    if (!TF_VERIFY(resourceRegistry)) {
        return;
    }

    const HdStTextureIdentifier textureId(
        TfToken(resolvedPath),
        std::make_unique<HdStAssetUvSubtextureIdentifier>(
            /* flipVertically = */ true,
            /* premultiplyAlpha = */ false,
	        /* sourceColorSpace = */ HdStTokens->colorSpaceAuto));

    static const HdSamplerParameters envSamplerParameters{
        HdWrapRepeat, HdWrapClamp, HdWrapClamp,
        HdMinFilterLinearMipmapLinear, HdMagFilterLinear};

    _domeLightEnvironmentTextureHandle =
        resourceRegistry->AllocateTextureHandle(
            textureId,
            HdTextureType::Uv,
            envSamplerParameters,
            /* targetMemory = */ 0,
            /* createBindlessHandle = */ false,
            shared_from_this());

    _namedTextureHandles = {
        _MakeNamedTextureHandle(
            _tokens->domeLightIrradiance,
            resolvedPath,
            HdWrapRepeat, HdWrapClamp, HdWrapRepeat,
            HdMinFilterLinear,
            resourceRegistry,
            shared_from_this()),

        _MakeNamedTextureHandle(
            _tokens->domeLightPrefilter,
            resolvedPath,
            HdWrapRepeat, HdWrapClamp, HdWrapRepeat,
            HdMinFilterLinearMipmapLinear,
            resourceRegistry,
            shared_from_this()),

        _MakeNamedTextureHandle(
            _tokens->domeLightBRDF,
            resolvedPath,
            HdWrapClamp, HdWrapClamp, HdWrapClamp,
            HdMinFilterLinear,
            resourceRegistry,
            shared_from_this())
    };
}

void
HdStSimpleLightingShader::AddResourcesFromTextures(ResourceContext &ctx) const
{
    if (!_domeLightEnvironmentTextureHandle) {
        // No dome lights, bail.
        return;
    }

    // Non-const weak pointer of this
    HdStSimpleLightingShaderPtr const thisShader =
        std::dynamic_pointer_cast<HdStSimpleLightingShader>(
            std::const_pointer_cast<HdStShaderCode, const HdStShaderCode>(
                shared_from_this()));

    // Irriadiance map computations.
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightIrradiance,
            thisShader),
        HdStComputeQueueZero);
    
    // Calculate the number of mips for the prefilter texture
    // Note that the size of the prefilter texture is half the size of the 
    // original Environment Map (srcTextureObject)
    const HdStUvTextureObject * const srcTextureObject = 
        dynamic_cast<HdStUvTextureObject*>(
            _domeLightEnvironmentTextureHandle->GetTextureObject().get());
    if (!TF_VERIFY(srcTextureObject)) {
        return;
    }
    const HgiTexture * const srcTexture = srcTextureObject->GetTexture().Get();
    if (!TF_VERIFY(srcTexture)) {
        return;
    }
    const GfVec3i srcDim = srcTexture->GetDescriptor().dimensions;

    const unsigned int numPrefilterLevels = 
        (unsigned int) std::log2(std::max(srcDim[0], srcDim[1]));

    // Prefilter map computations. mipLevel = 0 allocates texture.
    for (unsigned int mipLevel = 0; mipLevel < numPrefilterLevels; ++mipLevel) {
        const float roughness =
            (float)mipLevel / (float)(numPrefilterLevels - 1);

        ctx.AddComputation(
            nullptr,
            std::make_shared<HdSt_DomeLightComputationGPU>(
                _tokens->domeLightPrefilter, 
                thisShader,
                numPrefilterLevels,
                mipLevel,
                roughness),
            HdStComputeQueueZero);
    }

    // Brdf map computation
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightBRDF,
            thisShader),
        HdStComputeQueueZero);
}

HdStShaderCode::NamedTextureHandleVector const &
HdStSimpleLightingShader::GetNamedTextureHandles() const
{
    return _namedTextureHandles;
}

PXR_NAMESPACE_CLOSE_SCOPE

