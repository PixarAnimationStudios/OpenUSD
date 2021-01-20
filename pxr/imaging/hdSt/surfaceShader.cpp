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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/materialParam.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HDST_ENABLE_MATERIAL_PRIMVAR_FILTERING, true,
    "Enables filtering of primvar signals by material binding.");

static bool
_IsEnabledMaterialPrimvarFiltering() {
    return TfGetEnvSetting(HDST_ENABLE_MATERIAL_PRIMVAR_FILTERING);
}

static TfTokenVector
_CollectPrimvarNames(const HdSt_MaterialParamVector &params);

HdStSurfaceShader::HdStSurfaceShader()
 : HdStShaderCode()
 , _fragmentSource()
 , _geometrySource()
 , _params()
 , _paramSpec()
 , _paramArray()
 , _primvarNames(_CollectPrimvarNames(_params))
 , _isEnabledPrimvarFiltering(_IsEnabledMaterialPrimvarFiltering())
 , _computedHash(0)
 , _isValidComputedHash(false)
 , _computedTextureSourceHash(0)
 , _isValidComputedTextureSourceHash(false)
 , _materialTag()
{
}

HdStSurfaceShader::~HdStSurfaceShader() = default;

void
HdStSurfaceShader::_SetSource(TfToken const &shaderStageKey, std::string const &source)
{
    if (shaderStageKey == HdShaderTokens->fragmentShader) {
        _fragmentSource = source;
        _isValidComputedHash = false;
    } else if (shaderStageKey == HdShaderTokens->geometryShader) {
        _geometrySource = source;
        _isValidComputedHash = false;
    }
}

// -------------------------------------------------------------------------- //
// HdShader Virtual Interface                                                 //
// -------------------------------------------------------------------------- //

/*virtual*/
std::string
HdStSurfaceShader::GetSource(TfToken const &shaderStageKey) const
{
    if (shaderStageKey == HdShaderTokens->fragmentShader) {
        return _fragmentSource;
    } else if (shaderStageKey == HdShaderTokens->geometryShader) {
        return _geometrySource;
    }

    return std::string();
}
/*virtual*/
HdSt_MaterialParamVector const&
HdStSurfaceShader::GetParams() const
{
    return _params;
}
void
HdStSurfaceShader::SetEnabledPrimvarFiltering(bool enabled)
{
    _isEnabledPrimvarFiltering =
        enabled && _IsEnabledMaterialPrimvarFiltering();
}
/* virtual */
bool
HdStSurfaceShader::IsEnabledPrimvarFiltering() const
{
    return _isEnabledPrimvarFiltering;
}
/*virtual*/
TfTokenVector const&
HdStSurfaceShader::GetPrimvarNames() const
{
    return _primvarNames;
}
/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdStSurfaceShader::GetShaderData() const
{
    return _paramArray;
}

HdStShaderCode::NamedTextureHandleVector const &
HdStSurfaceShader::GetNamedTextureHandles() const
{
    return _namedTextureHandles;
}

/*virtual*/
void
HdStSurfaceShader::BindResources(const int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state)
{
    const bool bindlessTextureEnabled =
        GlfContextCaps::GetInstance().bindlessTextureEnabled;

    HdSt_TextureBinder::BindResources(
        binder, bindlessTextureEnabled, _namedTextureHandles);

    glActiveTexture(GL_TEXTURE0);

    binder.BindShaderResources(this);
}
/*virtual*/
void
HdStSurfaceShader::UnbindResources(const int program,
                                   HdSt_ResourceBinder const &binder,
                                   HdRenderPassState const &state)
{
    binder.UnbindShaderResources(this);

    const bool bindlessTextureEnabled =
        GlfContextCaps::GetInstance().bindlessTextureEnabled;

    HdSt_TextureBinder::UnbindResources(
        binder, bindlessTextureEnabled, _namedTextureHandles);

    glActiveTexture(GL_TEXTURE0);
}
/*virtual*/
void
HdStSurfaceShader::AddBindings(HdBindingRequestVector *customBindings)
{
}

/*virtual*/
HdStShaderCode::ID
HdStSurfaceShader::ComputeHash() const
{
    // All mutator methods that might affect the hash must reset this (fragile).
    if (!_isValidComputedHash) {
        _computedHash = _ComputeHash();
        _isValidComputedHash = true;
    }
    return _computedHash;
}

/*virtual*/
HdStShaderCode::ID
HdStSurfaceShader::ComputeTextureSourceHash() const
{
    if (!_isValidComputedTextureSourceHash) {
        _computedTextureSourceHash = _ComputeTextureSourceHash();
        _isValidComputedTextureSourceHash = true;
    }
    return _computedTextureSourceHash;
}

HdStShaderCode::ID
HdStSurfaceShader::_ComputeHash() const
{
    size_t hash = HdSt_MaterialParam::ComputeHash(_params);

    boost::hash_combine(hash, 
        ArchHash(_fragmentSource.c_str(), _fragmentSource.size()));
    boost::hash_combine(hash, 
        ArchHash(_geometrySource.c_str(), _geometrySource.size()));

    // Codegen is inspecting the shader bar spec to generate some
    // of the struct's, so we should probably use _paramSpec
    // in the hash computation as well.
    //
    // In practise, _paramSpec is generated from the
    // HdSt_MaterialParam's so the above is sufficient.

    return hash;
}

HdStShaderCode::ID
HdStSurfaceShader::_ComputeTextureSourceHash() const
{
    TRACE_FUNCTION();

    size_t hash = 0;

    for (const HdStShaderCode::NamedTextureHandle &namedHandle :
             _namedTextureHandles) {

        // Use name, texture object and sampling parameters.
        boost::hash_combine(hash, namedHandle.name);
        boost::hash_combine(hash, namedHandle.hash);
    }
    
    return hash;
}

void
HdStSurfaceShader::SetFragmentSource(const std::string &source)
{
    _fragmentSource = source;
    _isValidComputedHash = false;
}

void
HdStSurfaceShader::SetGeometrySource(const std::string &source)
{
    _geometrySource = source;
    _isValidComputedHash = false;
}

void
HdStSurfaceShader::SetParams(const HdSt_MaterialParamVector &params)
{
    _params = params;
    _primvarNames = _CollectPrimvarNames(_params);
    _isValidComputedHash = false;
}

void
HdStSurfaceShader::SetNamedTextureHandles(
    const NamedTextureHandleVector &namedTextureHandles)
{
    _namedTextureHandles = namedTextureHandles;
    _isValidComputedTextureSourceHash = false;
}

void
HdStSurfaceShader::SetBufferSources(
    HdBufferSpecVector const &bufferSpecs,
    HdBufferSourceSharedPtrVector &&bufferSources,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    if (bufferSpecs.empty()) {
        if (!_paramSpec.empty()) {
            _isValidComputedHash = false;
        }

        _paramSpec.clear();
        _paramArray.reset();
    } else {
        if (!_paramArray || _paramSpec != bufferSpecs) {
            _paramSpec = bufferSpecs;

            // establish a buffer range
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateShaderStorageBufferArrayRange(
                    HdTokens->materialParams,
                    bufferSpecs,
                    HdBufferArrayUsageHint());

            if (!TF_VERIFY(range->IsValid())) {
                _paramArray.reset();
            } else {
                _paramArray = range;
            }
            _isValidComputedHash = false;
        }

        if (_paramArray->IsValid()) {
            if (!bufferSources.empty()) {
                resourceRegistry->AddSources(_paramArray,
                                             std::move(bufferSources));
            }
        }
    }
}

TfToken
HdStSurfaceShader::GetMaterialTag() const
{
    return _materialTag;
}

void
HdStSurfaceShader::SetMaterialTag(TfToken const &tag)
{
    _materialTag = tag;
    _isValidComputedHash = false;
}

/// If the prim is based on asset, reload that asset.
void
HdStSurfaceShader::Reload()
{
    // Nothing to do, this shader's sources are externally managed.
}

/*static*/
bool
HdStSurfaceShader::CanAggregate(HdStShaderCodeSharedPtr const &shaderA,
                                HdStShaderCodeSharedPtr const &shaderB)
{
    // Can aggregate if the shaders are identical.
    if (shaderA == shaderB) {
        return true;
    }

    HdBufferArrayRangeSharedPtr dataA = shaderA->GetShaderData();
    HdBufferArrayRangeSharedPtr dataB = shaderB->GetShaderData();

    bool dataIsAggregated = (dataA == dataB) ||
                            (dataA && dataA->IsAggregatedWith(dataB));

    // We can't aggregate if the shaders have data buffers that aren't
    // aggregated or if the shaders don't match.
    if (!dataIsAggregated || shaderA->ComputeHash() != shaderB->ComputeHash()) {
        return false;
    }

    if (!GlfContextCaps::GetInstance().bindlessTextureEnabled) {
        if (shaderA->ComputeTextureSourceHash() !=
                shaderB->ComputeTextureSourceHash()) {
            return false;
        }
    }

    return true;
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (ptexFaceOffset)            // geometric shader

    (displayMetallic)           // simple lighting shader
    (displayRoughness)          // simple lighting shader

    (hullColor)                 // terminal shader
    (hullOpacity)               // terminal shader
    (scalarOverride)            // terminal shader
    (scalarOverrideColorRamp)   // terminal shader
    (selectedWeight)            // terminal shader

    (indicatorColor)            // renderPass shader
    (indicatorWeight)           // renderPass shader
    (overrideColor)             // renderPass shader
    (overrideWireframeColor)    // renderPass shader
    (maskColor)                 // renderPass shader
    (maskWeight)                // renderPass shader
    (wireframeColor)            // renderPass shader
);

static TfTokenVector const &
_GetExtraIncludedShaderPrimvarNames()
{
    static const TfTokenVector primvarNames = {
        HdTokens->displayColor,
        HdTokens->displayOpacity,

        // Include a few ad hoc primvar names that
        // are used by the built-in material shading system.

        _tokens->ptexFaceOffset,

        _tokens->displayMetallic,
        _tokens->displayRoughness,

        _tokens->hullColor,
        _tokens->hullOpacity,
        _tokens->scalarOverride,
        _tokens->scalarOverrideColorRamp,
        _tokens->selectedWeight,

        _tokens->indicatorColor,
        _tokens->indicatorWeight,
        _tokens->overrideColor,
        _tokens->overrideWireframeColor,
        _tokens->maskColor,
        _tokens->maskWeight,
        _tokens->wireframeColor
    };
    return primvarNames;
}

static TfTokenVector
_CollectPrimvarNames(const HdSt_MaterialParamVector &params)
{
    TfTokenVector primvarNames = _GetExtraIncludedShaderPrimvarNames();

    for (HdSt_MaterialParam const &param: params) {
        if (param.IsFallback()) {
            primvarNames.push_back(param.name);
        } else if (param.IsPrimvarRedirect()) {
            primvarNames.push_back(param.name);
            // primvar redirect connections are encoded as sampler coords
            primvarNames.insert(primvarNames.end(),
                                param.samplerCoords.begin(),
                                param.samplerCoords.end());
        } else if (param.IsTexture()) {
            // include sampler coords for textures
            primvarNames.insert(primvarNames.end(),
                                param.samplerCoords.begin(),
                                param.samplerCoords.end());
        } else if (param.IsAdditionalPrimvar()) {
            primvarNames.push_back(param.name);
        }
    }
    return primvarNames;
}

void
HdStSurfaceShader::AddResourcesFromTextures(ResourceContext &ctx) const
{
    const bool bindlessTextureEnabled =
        GlfContextCaps::GetInstance().bindlessTextureEnabled;

    // Add buffer sources for bindless texture handles (and
    // other texture metadata such as the sampling transform for
    // a field texture).
    HdBufferSourceSharedPtrVector result;
    HdSt_TextureBinder::ComputeBufferSources(
        GetNamedTextureHandles(), bindlessTextureEnabled, &result);

    if (!result.empty()) {
        ctx.AddSources(GetShaderData(), std::move(result));
    }
}

void
HdStSurfaceShader::AddFallbackValueToSpecsAndSources(
    const HdSt_MaterialParam &param,
    HdBufferSpecVector * const specs,
    HdBufferSourceSharedPtrVector * const sources)
{
    const TfToken sourceName(
        param.name.GetString()
        + HdSt_ResourceBindingSuffixTokens->fallback.GetString());

    HdBufferSourceSharedPtr const source =
        std::make_shared<HdVtBufferSource>(
            sourceName, param.fallbackValue);
    source->GetBufferSpecs(specs);
    sources->push_back(std::move(source));
}

PXR_NAMESPACE_CLOSE_SCOPE
