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
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/domeLightComputations.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hgiGL/texture.h"

#include "pxr/imaging/glf/bindingMap.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <string>
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
    , _domeLightIrradianceGLName(0)
    , _domeLightPrefilterGLName(0)
    , _domeLightBrdfGLName(0)
    , _domeLightIrradianceGLSampler(0)
    , _domeLightPrefilterGLSampler(0)
    , _domeLightBrdfGLSampler(0)
{
    _lightingContext->InitUniformBlockBindings(_bindingMap);
    _lightingContext->InitSamplerUnitBindings(_bindingMap);
}

HdStSimpleLightingShader::~HdStSimpleLightingShader()
{
    if (_domeLightIrradianceGLName) {
        glDeleteTextures(1, &_domeLightIrradianceGLName);
    }
    if (_domeLightPrefilterGLName) {
        glDeleteTextures(1, &_domeLightPrefilterGLName);
    }
    if (_domeLightBrdfGLName) {
        glDeleteTextures(1, &_domeLightBrdfGLName);
    }

    if (_domeLightIrradianceGLSampler) {
        glDeleteSamplers(1, &_domeLightIrradianceGLSampler);
    }
    if (_domeLightPrefilterGLSampler) {
        glDeleteSamplers(1, &_domeLightPrefilterGLSampler);
    }
    if (_domeLightBrdfGLSampler) {
        glDeleteSamplers(1, &_domeLightBrdfGLSampler);
    }
}

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

    return defineStream.str() + source;
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

static
void
_BindTextureAndSampler(HdSt_ResourceBinder const &binder,
                       TfToken const &token,
                       const uint32_t glName,
                       const uint32_t glSampler)
{
    const HdBinding binding = binder.GetBinding(token);
    if (binding.GetType() != HdBinding::TEXTURE_2D) {
        return;
    }
    
    const int samplerUnit = binding.GetTextureUnit();
    glActiveTexture(GL_TEXTURE0 + samplerUnit);
    glBindTexture(GL_TEXTURE_2D, glName);
    glBindSampler(samplerUnit, glSampler);
}

/* virtual */
void
HdStSimpleLightingShader::BindResources(const int program,
                                        HdSt_ResourceBinder const &binder,
                                        HdRenderPassState const &state)
{
    // XXX: we'd like to use HdSt_ResourceBinder instead of GlfBindingMap.
    //
    _bindingMap->AssignUniformBindingsToProgram(program);
    _lightingContext->BindUniformBlocks(_bindingMap);

    _bindingMap->AssignSamplerUnitsToProgram(program);
    _lightingContext->BindSamplers(_bindingMap);

    if(_HasDomeLight(_lightingContext)) {
        _BindTextureAndSampler(binder,
                               _tokens->domeLightIrradiance,
                               _domeLightIrradianceGLName,
                               _domeLightIrradianceGLSampler);
        _BindTextureAndSampler(binder,
                               _tokens->domeLightPrefilter,
                               _domeLightPrefilterGLName,
                               _domeLightPrefilterGLSampler);
        _BindTextureAndSampler(binder,
                               _tokens->domeLightBRDF,
                               _domeLightBrdfGLName,
                               _domeLightBrdfGLSampler);
    }
    glActiveTexture(GL_TEXTURE0);
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

    if(_HasDomeLight(_lightingContext)) {
        _BindTextureAndSampler(binder,
                               _tokens->domeLightIrradiance,
                               0,
                               0);
        _BindTextureAndSampler(binder,
                               _tokens->domeLightPrefilter,
                               0,
                               0);
        _BindTextureAndSampler(binder,
                               _tokens->domeLightBRDF,
                               0,
                               0);
    }

    glActiveTexture(GL_TEXTURE0);
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

void
HdStSimpleLightingShader::AllocateTextureHandles(HdSceneDelegate *const delegate)
{
    SdfAssetPath path;

    for (auto const& light : _lightingContext->GetLights()){
        if (light.IsDomeLight()) {
            path = light.GetDomeLightTextureFile();
        }
    }

    const std::string &resolvedPath = path.GetResolvedPath();
    if (resolvedPath.empty()) {
        _domeLightTextureHandle = nullptr;
        return;
    }

    if (_domeLightTextureHandle) {
        HdStTextureIdentifier const &textureId =
            _domeLightTextureHandle->GetTextureObject()->GetTextureIdentifier();
        if (textureId.GetFilePath() != resolvedPath) {
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
        std::make_unique<HdStUvOrientationSubtextureIdentifier>(
            /* flipVertically = */ true));

    static const HdSamplerParameters samplerParameters{
        HdWrapRepeat, HdWrapRepeat, HdWrapRepeat,
        HdMinFilterLinear, HdMagFilterLinear};

    _domeLightTextureHandle = resourceRegistry->AllocateTextureHandle(
        textureId, 
        HdTextureType::Uv,
        samplerParameters,
        /* targetMemory = */ 0,
        /* createBindlessHandle = */ false,
        shared_from_this());
}

void
HdStSimpleLightingShader::AddResourcesFromTextures(ResourceContext &ctx) const
{
    if (!_domeLightTextureHandle) {
        // No dome lights, bail.
        return;
    }
    
    // Get the GL name of the environment map that
    // was loaded during commit.
    HdStTextureObjectSharedPtr const &textureObject =
        _domeLightTextureHandle->GetTextureObject();
    HdStUvTextureObject const *uvTextureObject =
        dynamic_cast<HdStUvTextureObject*>(
            textureObject.get());
    if (!TF_VERIFY(uvTextureObject)) {
        return;
    }
    HgiTexture* const texture = uvTextureObject->GetTexture().Get();
    HgiGLTexture* const glTexture = dynamic_cast<HgiGLTexture*>(texture);
    if (!TF_VERIFY(glTexture)) {
        return;
    }
    const uint32_t glTextureName = glTexture->GetTextureId();

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
            glTextureName,
            thisShader));
    
    static const GLuint numPrefilterLevels = 5;

    // Prefilter map computations. mipLevel = 0 allocates texture.
    for (unsigned int mipLevel = 0; mipLevel < numPrefilterLevels; ++mipLevel) {
        const float roughness =
            (float)mipLevel / (float)(numPrefilterLevels - 1);

        ctx.AddComputation(
            nullptr,
            std::make_shared<HdSt_DomeLightComputationGPU>(
                _tokens->domeLightPrefilter, 
                glTextureName,
                thisShader,
                numPrefilterLevels,
                mipLevel,
                roughness));
    }

    // Brdf map computation
    ctx.AddComputation(
        nullptr,
        std::make_shared<HdSt_DomeLightComputationGPU>(
            _tokens->domeLightBRDF,
            glTextureName,
            thisShader));
}

static
void
_CreateSampler(uint32_t * const samplerName,
               const GLenum wrapMode,
               const GLenum minFilter)
{
    if (*samplerName) {
        return;
    }
    glGenSamplers(1, samplerName);
    glSamplerParameteri(*samplerName, GL_TEXTURE_WRAP_S, wrapMode);
    glSamplerParameteri(*samplerName, GL_TEXTURE_WRAP_T, wrapMode);
    glSamplerParameteri(*samplerName, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(*samplerName, GL_TEXTURE_MIN_FILTER, minFilter);
}

void
HdStSimpleLightingShader::_CreateSamplersIfNecessary()
{
    _CreateSampler(&_domeLightIrradianceGLSampler,
                   GL_REPEAT,
                   GL_LINEAR);
    _CreateSampler(&_domeLightPrefilterGLSampler,
                   GL_REPEAT,
                   GL_LINEAR_MIPMAP_LINEAR);
    _CreateSampler(&_domeLightBrdfGLSampler,
                   GL_CLAMP_TO_EDGE,
                   GL_LINEAR);
}

void
HdStSimpleLightingShader::SetGLTextureName(
    const TfToken &token, const uint32_t glName)
{
    _CreateSamplersIfNecessary();

    if (token == _tokens->domeLightIrradiance) {
        _domeLightIrradianceGLName = glName;
    } else if (token == _tokens->domeLightPrefilter) {
        _domeLightPrefilterGLName = glName;
    } else if (token == _tokens->domeLightBRDF) {
        _domeLightBrdfGLName = glName;
    } else {
        TF_CODING_ERROR("Unsupported texture token %s", token.GetText());
    }
}

uint32_t
HdStSimpleLightingShader::GetGLTextureName(const TfToken &token) const
{
    if (token == _tokens->domeLightIrradiance) {
        return _domeLightIrradianceGLName;
    }
    if (token == _tokens->domeLightPrefilter) {
        return _domeLightPrefilterGLName;
    }
    if (token == _tokens->domeLightBRDF) {
        return _domeLightBrdfGLName;
    }
    TF_CODING_ERROR("Unsupported texture token %s", token.GetText());
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE

