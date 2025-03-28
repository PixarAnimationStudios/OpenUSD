//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/binding.h"
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
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/tf/hash.h"

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
    , _useLighting(true)
    , _glslfx(std::make_unique<HioGlslfx>(HdStPackageSimpleLightingShader()))
    , _renderParam(nullptr)
{
}

HdStSimpleLightingShader::~HdStSimpleLightingShader()
{
    _CleanupAovBindings();
};

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
    hash = TfHash::Combine(
        hash,
        numLights,
        useShadows,
        numShadows,
        _lightingContext->ComputeShaderSourceHash()
    );

    for (const HdStShaderCode::NamedTextureHandle &namedHandle :
        _namedTextureHandles) {
        
        // Use name and hash only - not the texture itself as this
        // does not affect the generated shader source.
        hash = TfHash::Combine(
            hash,
            namedHandle.name,
            namedHandle.hash
        );
    }

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
                                        HdSt_ResourceBinder const &binder)
{
    for (auto const& buffer : _customBuffers) {
        binder.Bind(buffer.second);
    }
    
    HdSt_TextureBinder::BindResources(binder, _namedTextureHandles);
}

/* virtual */
void
HdStSimpleLightingShader::UnbindResources(const int program,
                                          HdSt_ResourceBinder const &binder)
{
    for (auto const& buffer : _customBuffers) {
        binder.Unbind(buffer.second);
    }

    HdSt_TextureBinder::UnbindResources(binder, _namedTextureHandles);
}

void
HdStSimpleLightingShader::AddBufferBinding(HdStBindingRequest const& req)
{
    _customBuffers[req.GetName()] = req;
}

void
HdStSimpleLightingShader::RemoveBufferBinding(TfToken const &name)
{
    _customBuffers.erase(name);
}

void
HdStSimpleLightingShader::ClearBufferBindings()
{
    _customBuffers.clear();
}

/*virtual*/
void
HdStSimpleLightingShader::AddBindings(HdStBindingRequestVector *customBindings)
{
    customBindings->reserve(customBindings->size() + _customBuffers.size() + 1);
    TF_FOR_ALL(it, _customBuffers) {
        customBindings->push_back(it->second);
    }

    _lightTextureParams.clear();
    
    // For now we assume that the only simple light with a texture is
    // a domeLight (ignoring RectLights, and multiple domeLights)
    if (_HasDomeLight(_lightingContext) && _domeLightEnvironmentTextureHandle) {
        // irradiance map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightIrradiance,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Uv));
        // prefilter map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightPrefilter,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Uv));
        // BRDF texture
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightBRDF,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Uv));
    }

    const bool useShadows =
        _useLighting ? _lightingContext->GetUseShadows() : false;
    if (useShadows) {
        size_t const numShadowPasses = 
            _lightingContext->GetShadows()->GetNumShadowMapPasses();

        // Create one param for all shadow passes as shadow compare textures 
        // will be bound to shader as an array of samplers.
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                HdStTokens->shadowCompareTextures,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Uv,
                /*swizzle*/std::string(),
                /*isPremultiplied*/false,
                /*arrayOfTexturesSize*/numShadowPasses));
    }
}

HdSt_MaterialParamVector const& 
HdStSimpleLightingShader::GetParams() const 
{
    return _lightTextureParams;
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
            const SdfAssetPath &path = it->GetDomeLightTextureFile();
            const std::string &assetPath = path.GetAssetPath();
            if (assetPath.empty()) {
                TF_WARN("Dome light has no texture asset path.");
                return empty;
            }

            const std::string &resolvedPath = path.GetResolvedPath();
            if (resolvedPath.empty()) {
                TF_WARN("Texture asset path '%s' for dome light could not be resolved.",
                        assetPath.c_str());
            }
            return resolvedPath;
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

    const HdSamplerParameters samplerParameters(
        wrapModeS, wrapModeT, wrapModeR,
        minFilter, HdMagFilterLinear,
        HdBorderColorTransparentBlack,
        /*enableCompare*/false, HdCmpFuncNever,
        /*maxAnisotropy*/1);

    HdStTextureHandleSharedPtr const textureHandle =
        resourceRegistry->AllocateTextureHandle(
            textureId,
            HdStTextureType::Uv,
            samplerParameters,
            /* memoryRequest = */ 0,
            shader);

    return { name,
             HdStTextureType::Uv,
             textureHandle,
             name.Hash() };
}

SdfPath
HdStSimpleLightingShader::_GetAovPath(
    TfToken const &aovName, size_t shadowIndex) const
{
    std::string identifier = std::string("aov_shadowMap") +
        std::to_string(shadowIndex) + "_" + 
        TfMakeValidIdentifier(aovName.GetString());
    return SdfPath(identifier);
}

void
HdStSimpleLightingShader::_ResizeOrCreateBufferForAov(size_t shadowIndex) const
{
    GlfSimpleShadowArrayRefPtr const& shadows = _lightingContext->GetShadows();

    GfVec3i const dimensions = GfVec3i(
        shadows->GetShadowMapSize(shadowIndex)[0], 
        shadows->GetShadowMapSize(shadowIndex)[1], 
        1);

    HdRenderPassAovBinding const & aovBinding = _shadowAovBindings[shadowIndex];
    VtValue existingResource = aovBinding.renderBuffer->GetResource(false);
    if (existingResource.IsHolding<HgiTextureHandle>()) {
        int32_t const width = aovBinding.renderBuffer->GetWidth();
        int32_t const height = aovBinding.renderBuffer->GetHeight();
        if (width == dimensions[0] && height == dimensions[1]) {
            return;
        }
    }

    // If the resolution has changed then reallocate the
    // renderBuffer and  texture.
    aovBinding.renderBuffer->Allocate(dimensions,
                                      HdFormatFloat32,
                                      /*multiSampled*/false);

    VtValue newResource = aovBinding.renderBuffer->GetResource(false);

    if (!newResource.IsHolding<HgiTextureHandle>()) {
        TF_CODING_ERROR("No texture on render buffer for AOV "
                        "%s", aovBinding.aovName.GetText());
    }
}

void
HdStSimpleLightingShader::_CleanupAovBindings()
{
    if (_renderParam) {
        for (auto const & aovBuffer : _shadowAovBuffers) {
            aovBuffer->Finalize(_renderParam);
        }
    }
    _shadowAovBuffers.clear();
    _shadowAovBindings.clear();
}

void
HdStSimpleLightingShader::AllocateTextureHandles(HdRenderIndex const &renderIndex)
{
    const std::string &resolvedPath =
        _GetResolvedDomeLightEnvironmentFilePath(_lightingContext);
    const bool useShadows =
        _useLighting ? _lightingContext->GetUseShadows() : false;
    if (resolvedPath.empty()) {
        _domeLightEnvironmentTextureHandle = nullptr;
        _domeLightTextureHandles.clear();
    }

    if (!useShadows) {
        _CleanupAovBindings();
        _shadowTextureHandles.clear();
    }

    if (resolvedPath.empty() && !useShadows) {
        _namedTextureHandles.clear();
        return;
    }

    bool recomputeDomeLightTextures = !resolvedPath.empty();
    if (_domeLightEnvironmentTextureHandle) {
        HdStTextureObjectSharedPtr const &textureObject =
            _domeLightEnvironmentTextureHandle->GetTextureObject();
        HdStTextureIdentifier const &textureId =
            textureObject->GetTextureIdentifier();
        if (textureId.GetFilePath() == resolvedPath) {
            // Same environment map, no need to recompute
            // dome light textures.
            recomputeDomeLightTextures = false;
        }
    }

    // Store render index for render buffer destruction.
    _renderParam = renderIndex.GetRenderDelegate()->GetRenderParam();

    HdStResourceRegistry * const resourceRegistry =
        dynamic_cast<HdStResourceRegistry*>(
            renderIndex.GetResourceRegistry().get());
    if (!TF_VERIFY(resourceRegistry)) {
        return;
    }

    // Allocate texture handles for dome light textures.
    if (recomputeDomeLightTextures) {
        _domeLightTextureHandles.clear();

        const HdStTextureIdentifier textureId(
            TfToken(resolvedPath),
            std::make_unique<HdStAssetUvSubtextureIdentifier>(
                /* flipVertically = */ true,
                /* premultiplyAlpha = */ false,
                /* sourceColorSpace = */ HdStTokens->colorSpaceAuto));

        static const HdSamplerParameters envSamplerParameters(
            HdWrapRepeat, HdWrapClamp, HdWrapClamp,
            HdMinFilterLinearMipmapLinear, HdMagFilterLinear,
            HdBorderColorTransparentBlack,
            /*enableCompare*/false, HdCmpFuncNever,
            /*maxAnisotropy*/1);

        _domeLightEnvironmentTextureHandle =
            resourceRegistry->AllocateTextureHandle(
                textureId,
                HdStTextureType::Uv,
                envSamplerParameters,
                /* targetMemory = */ 0,
                shared_from_this());

        _domeLightTextureHandles = {
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
    _namedTextureHandles = _domeLightTextureHandles;

    // Allocate texture handles for shadow map textures.
    if (useShadows) {     
        GlfSimpleShadowArrayRefPtr const& shadows = 
            _lightingContext->GetShadows();
        size_t const prevNumShadowPasses = _shadowAovBindings.size();
        size_t const numShadowPasses = shadows->GetNumShadowMapPasses();

        if (prevNumShadowPasses < numShadowPasses) {
            // If increasing number of shadow maps, need to create new
            // aov bindings and render buffers.
            _shadowAovBindings.resize(numShadowPasses);

            for (size_t i = prevNumShadowPasses; i < numShadowPasses; i++) {
                SdfPath const aovId = _GetAovPath(HdAovTokens->depth, i);
                _shadowAovBuffers.push_back(
                    std::make_unique<HdStRenderBuffer>(
                        resourceRegistry, aovId));
                    
                HdAovDescriptor aovDesc = HdAovDescriptor(HdFormatFloat32, 
                                                          /*multiSampled*/false,
                                                          VtValue(1.f));

                HdRenderPassAovBinding &binding = _shadowAovBindings[i];
                binding.aovName = HdAovTokens->depth;
                binding.aovSettings = aovDesc.aovSettings;
                binding.renderBufferId = aovId;
                binding.clearValue = aovDesc.clearValue;
                binding.renderBuffer = _shadowAovBuffers.back().get();
            }
        } else if (prevNumShadowPasses > numShadowPasses) {
            // If decreasing number of shadow maps, only need to finalize 
            // and resize.
            if (_renderParam) {
                for (size_t i = numShadowPasses; i < prevNumShadowPasses; i++) {
                    _shadowAovBuffers[i]->Finalize(_renderParam);
                }
            }
            _shadowAovBindings.resize(numShadowPasses);
            _shadowAovBuffers.resize(numShadowPasses);
        }
        
        for (size_t i = 0; i < numShadowPasses; i++) {
            _ResizeOrCreateBufferForAov(i);
        }

        if (prevNumShadowPasses < numShadowPasses) {
            // If increasing number of shadow maps, allocate texture handles
            // for just-allocated texture objects.
            HdSamplerParameters const shadowSamplerParameters{
                HdWrapClamp, HdWrapClamp, HdWrapClamp,
                HdMinFilterLinear, HdMagFilterLinear,
                HdBorderColorOpaqueWhite, /*enableCompare*/true, 
                HdCmpFuncLEqual, /*maxAnisotropy*/16};

            for (size_t i = prevNumShadowPasses; i < numShadowPasses; i++) {
                HdStTextureHandleSharedPtr const textureHandle =
                    resourceRegistry->AllocateTextureHandle(
                        _shadowAovBuffers[i]->GetTextureIdentifier(false),
                        HdStTextureType::Uv,
                        shadowSamplerParameters,
                        /* memoryRequest = */ 0,
                        shared_from_this());

                TfToken const shadowTextureName = TfToken(
                    HdStTokens->shadowCompareTextures.GetString() + 
                    std::to_string(i));
                _shadowTextureHandles.push_back(
                    NamedTextureHandle{ 
                        shadowTextureName,
                        HdStTextureType::Uv,
                        textureHandle,
                        shadowTextureName.Hash()});
            }
        } else if (prevNumShadowPasses > numShadowPasses) {
            _shadowTextureHandles.resize(numShadowPasses);
        }
    }

    _namedTextureHandles.insert(
        _namedTextureHandles.end(), 
        _shadowTextureHandles.begin(),
        _shadowTextureHandles.end());
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
    if (!srcTexture) {
        TF_WARN(
            "Invalid texture for dome light environment map at %s",
            srcTextureObject->GetTextureIdentifier().GetFilePath().GetText());
        return;
    }
    const GfVec3i srcDim = srcTexture->GetDescriptor().dimensions;

    const unsigned int numPrefilterLevels = 
        std::max((unsigned int)(std::log2(std::max(srcDim[0], srcDim[1]))), 1u);

    // Prefilter map computations. mipLevel = 0 allocates texture.
    for (unsigned int mipLevel = 0; mipLevel < numPrefilterLevels; ++mipLevel) {
        const float roughness = (numPrefilterLevels == 1) ? 0.f :
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

