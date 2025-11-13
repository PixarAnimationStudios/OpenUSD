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
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/tf/hash.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (domeLightCubemap)
    (domeLightIrradiance)
    (domeLightPrefilter) 
    (domeLightBRDF)
);


HdStSimpleLightingShader::HdStSimpleLightingShader() 
    : _lightingContext(GlfSimpleLightingContext::New())
    , _useLighting(true)
    , _glslfx(std::make_unique<HioGlslfx>(HdStPackageSimpleLightingShader()))
    , _domeLightCubemapTargetMemoryMB(0)
    , _shadowTextureHandle(
        NamedTextureHandle{ 
            HdStTokens->shadowCompareTextures,
            HdStTextureType::Uv,
            {},
            HdStTokens->shadowCompareTextures.Hash()})
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
        // cubemap generated from texture
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightCubemap,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Cubemap));
        // irradiance map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightIrradiance,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Cubemap));
        // prefilter map
        _lightTextureParams.push_back(
            HdSt_MaterialParam(
                HdSt_MaterialParam::ParamTypeTexture,
                _tokens->domeLightPrefilter,
                VtValue(GfVec4f(0.0)),
                TfTokenVector(),
                HdStTextureType::Cubemap));
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
    // This is used specfically for dome lights, so ok to just return first 
    // handle.
    for (auto const & namedTextureHandle : _namedTextureHandles) {
        if (namedTextureHandle.name == name) {
            return namedTextureHandle.handles[0];
        }
    }

    static const HdStTextureHandleSharedPtr empty;
    return empty;
}

const HdStTextureHandleSharedPtr &
HdStSimpleLightingShader::GetDomeLightEnvironmentCubemapTextureHandle() const
{
    return GetTextureHandle(_tokens->domeLightCubemap);
}

namespace {

template<HdStTextureType textureType>
struct _SubtextureIdTypeHelper;

template<HdStTextureType textureType>
using _SubtextureIdType =
    typename _SubtextureIdTypeHelper<textureType>::type;

template<>
struct _SubtextureIdTypeHelper<HdStTextureType::Uv> {
    using type = HdStDynamicUvSubtextureIdentifier;
};

template<>
struct _SubtextureIdTypeHelper<HdStTextureType::Cubemap> {
    using type = HdStDynamicCubemapSubtextureIdentifier;
};

template <HdStTextureType textureType>
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
        std::make_unique<_SubtextureIdType<textureType>>());

    const HdSamplerParameters samplerParameters(
        wrapModeS, wrapModeT, wrapModeR,
        minFilter, HdMagFilterLinear,
        HdBorderColorTransparentBlack,
        /*enableCompare*/false, HdCmpFuncNever,
        /*maxAnisotropy*/1);

    HdStTextureHandleSharedPtr const textureHandle =
        resourceRegistry->AllocateTextureHandle(
            textureId,
            textureType,
            samplerParameters,
            /* memoryRequest = */ 0,
            shader);

    return { name,
             textureType,
             { textureHandle },
             name.Hash() };
}

}

void
HdStSimpleLightingShader::_CleanupAovBindings()
{
    _shadowAovBindings.clear();
    _shadowTextureHandle.handles.clear();
}

void
HdStSimpleLightingShader::AllocateTextureHandles(
    HdRenderIndex const &renderIndex,
    const SdfPath& graphPath)
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
            _MakeNamedTextureHandle<HdStTextureType::Cubemap>(
                _tokens->domeLightCubemap,
                resolvedPath,
                // Wrap modes irrelevant for seamless cubemaps
                HdWrapClamp, HdWrapClamp, HdWrapClamp,
                HdMinFilterLinearMipmapLinear,
                resourceRegistry,
                shared_from_this()),

            _MakeNamedTextureHandle<HdStTextureType::Cubemap>(
                _tokens->domeLightIrradiance,
                resolvedPath,
                // Wrap modes irrelevant for seamless cubemaps
                HdWrapClamp, HdWrapClamp, HdWrapClamp,
                HdMinFilterLinear,
                resourceRegistry,
                shared_from_this()),

            _MakeNamedTextureHandle<HdStTextureType::Cubemap>(
                _tokens->domeLightPrefilter,
                resolvedPath,
                // Wrap modes irrelevant for seamless cubemaps
                HdWrapClamp, HdWrapClamp, HdWrapClamp,
                HdMinFilterLinearMipmapLinear,
                resourceRegistry,
                shared_from_this()),

            _MakeNamedTextureHandle<HdStTextureType::Uv>(
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
        _AllocateShadowTextures(resourceRegistry, graphPath);
        _namedTextureHandles.push_back(_shadowTextureHandle);
    }
}

void
HdStSimpleLightingShader::_AllocateShadowTextures(
        HdStResourceRegistry* const resourceRegistry,
        const SdfPath& graphPath)
{
    GlfSimpleShadowArrayRefPtr const& shadows = 
        _lightingContext->GetShadows();
    size_t const numShadowPasses = shadows->GetNumShadowMapPasses();
    _shadowAovBindings.resize(numShadowPasses);
    _shadowTextureHandle.handles.resize(numShadowPasses);
    _shadowBuffers.resize(numShadowPasses);

    for (uint32_t i = 0; i < numShadowPasses; i++) {
        const GfVec2i dimensions = shadows->GetShadowMapSize(i); 
        // Skip allocation if buffer already exists and is correct size
        if (_shadowAovBindings[i].renderBuffer) {
            const GfVec2i bindingDims{
                static_cast<int>(
                    _shadowAovBindings[i].renderBuffer->GetWidth()),
                static_cast<int>(
                    _shadowAovBindings[i].renderBuffer->GetHeight())
            };
            if (bindingDims == dimensions) {
                continue;
            }
        }

        _shadowBuffers[i] = 
            resourceRegistry->AllocateTempRenderBuffer(
                graphPath,
                HdFormatFloat32,
                dimensions,
                /*multiSampled=*/false,
                /*depth=*/true);

        HdAovDescriptor aovDesc = HdAovDescriptor(HdFormatFloat32, 
                                                    /*multiSampled*/false,
                                                    /*c=*/VtValue(1.f));

        HdRenderPassAovBinding &binding = _shadowAovBindings[i];
        binding.aovName = HdAovTokens->depth;
        binding.aovSettings = aovDesc.aovSettings;
        binding.renderBufferId = _shadowBuffers[i]->GetBuffer()->GetId();
        binding.clearValue = aovDesc.clearValue;
        binding.renderBuffer = _shadowBuffers[i]->GetBuffer();

        HdSamplerParameters const shadowSamplerParameters{
            HdWrapClamp, HdWrapClamp, HdWrapClamp,
            HdMinFilterLinear, HdMagFilterLinear,
            HdBorderColorOpaqueWhite, /*enableCompare*/true, 
            HdCmpFuncLEqual, /*maxAnisotropy*/16};

        _shadowTextureHandle.handles[i] =
            resourceRegistry->AllocateTextureHandle(
                _shadowBuffers[i]->GetBuffer()->GetTextureIdentifier(false),
                HdStTextureType::Uv,
                shadowSamplerParameters,
                /* memoryRequest = */ 0,
                shared_from_this());
    }
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

    // Calculate the number of mips for the cubemap texture.
    const auto * const srcTextureObject = 
        dynamic_cast<HdStAssetUvTextureObject*>(
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

    const int cubemapDim =
        HdSt_ComputeDomeLightCubemapWidth(
            srcTextureObject->GetTextureIdentifier().GetFilePath().GetString(),
            srcTextureObject->GetTexture()->GetDescriptor(),
            _domeLightCubemapTargetMemoryMB);
    const auto numCubemapLevels = (unsigned int)(std::log2(cubemapDim) + 1);

    // Cubemap generation from latlong texture.
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightCubemap,
            /*useCubemapAsSourceTexture =*/false,
            thisShader,
            cubemapDim,
            numCubemapLevels),
        HdStComputeQueueZero
    );

    // BRDF computation.
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightBRDF,
            /*useCubemapAsSourceTexture =*/false,
            thisShader,
            cubemapDim),
        HdStComputeQueueZero
    );

    // Generate mipmaps the for the generated cubemap.
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightMipmapComputationGPU>(
            thisShader),
        HdStComputeQueueOne
    );

    // Irradiance map computation.
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightIrradiance,
            /*useCubemapAsSourceTexture =*/true,
            thisShader,
            cubemapDim),
        HdStComputeQueueTwo
    );

    // Note that since the prefilter texture is downsized, it has one less
    // mip level than the cubemap.
    const unsigned int numPrefilterLevels = std::max(numCubemapLevels - 1, 1u);

    // Prefilter map computations. mipLevel = 0 allocates texture.
    for (unsigned int mipLevel = 0; mipLevel < numPrefilterLevels; ++mipLevel) {
        const float roughness = (numPrefilterLevels == 1) ? 0.f :
            (float)mipLevel / (float)(numPrefilterLevels - 1);

        ctx.AddComputation(
            nullptr,
            std::make_shared<HdSt_DomeLightComputationGPU>(
                _tokens->domeLightPrefilter,
                /*useCubemapAsSourceTexture =*/true,
                thisShader,
                cubemapDim,
                numPrefilterLevels,
                mipLevel,
                roughness),
            HdStComputeQueueTwo
        );
    }
}

HdStShaderCode::NamedTextureHandleVector const &
HdStSimpleLightingShader::GetNamedTextureHandles() const
{
    return _namedTextureHandles;
}

void 
HdStSimpleLightingShader::SetDomeLightCubemapTargetMemory(
    unsigned int targetMemoryMB)
{
    if (_domeLightCubemapTargetMemoryMB != targetMemoryMB) {
        _domeLightCubemapTargetMemoryMB = targetMemoryMB;
        _domeLightEnvironmentTextureHandle = nullptr;
        _domeLightTextureHandles.clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

