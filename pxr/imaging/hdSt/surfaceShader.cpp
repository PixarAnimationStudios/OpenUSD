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

#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/contextCaps.h"

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
_CollectPrimvarNames(const HdMaterialParamVector &params);

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
 , _textureDescriptors()
 , _materialTag()
{
}

HdStSurfaceShader::~HdStSurfaceShader()
{
}

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
HdMaterialParamVector const&
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
/*virtual*/
HdStShaderCode::TextureDescriptorVector
HdStSurfaceShader::GetTextures() const
{
    return _textureDescriptors;
}
/*virtual*/
void
HdStSurfaceShader::BindResources(const int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state)
{
    for (auto const & it : _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it.name);

        if (!TF_VERIFY(it.handle)) {
            continue;
        }
        HdStTextureResourceSharedPtr resource = it.handle->GetTextureResource();

        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, resource->GetTexelsTextureId());
            glBindSampler(samplerUnit, resource->GetTexelsSamplerId());
            
            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        } else if (binding.GetType() == HdBinding::TEXTURE_3D) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_3D, resource->GetTexelsTextureId());
            glBindSampler(samplerUnit, resource->GetTexelsSamplerId());
            
            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        } else if (binding.GetType() == HdBinding::TEXTURE_UDIM_ARRAY) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, resource->GetTexelsTextureId());
            glBindSampler(samplerUnit, resource->GetTexelsSamplerId());

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        } else if (binding.GetType() == HdBinding::TEXTURE_UDIM_LAYOUT) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_1D, resource->GetLayoutTextureId());

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, resource->GetTexelsTextureId());

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, resource->GetLayoutTextureId());

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
        }
    }
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

    for (auto const & it : _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it.name);
        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindSampler(samplerUnit, 0);
        } else if (binding.GetType() == HdBinding::TEXTURE_3D) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_3D, 0);
            glBindSampler(samplerUnit, 0);
        } else if (binding.GetType() == HdBinding::TEXTURE_UDIM_ARRAY) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            glBindSampler(samplerUnit, 0);
        } else if (binding.GetType() == HdBinding::TEXTURE_UDIM_LAYOUT) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_1D, 0);
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            int samplerUnit = binding.GetTextureUnit();
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
        }
    }
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

HdStShaderCode::ID
HdStSurfaceShader::_ComputeHash() const
{
    size_t hash = 0;
    
    for (HdMaterialParam const& param : _params) {
        if (param.IsFallback())
            boost::hash_combine(hash, param.name.Hash());
    }
    boost::hash_combine(hash, 
        ArchHash(_fragmentSource.c_str(), _fragmentSource.size()));
    boost::hash_combine(hash, 
        ArchHash(_geometrySource.c_str(), _geometrySource.size()));

    // Add in texture format that effects shader (ignore handles)
    boost::hash_combine(hash, _textureDescriptors.size());
    for (TextureDescriptorVector::const_iterator
                                        texIt  = _textureDescriptors.cbegin();
                                        texIt != _textureDescriptors.cend();
                                      ++texIt)
    {
        const TextureDescriptor &texDesc = *texIt;

        boost::hash_combine(hash, texDesc.name);
        boost::hash_combine(hash, texDesc.type);
    }

    boost::hash_combine(hash, _materialTag.Hash());

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
HdStSurfaceShader::SetParams(const HdMaterialParamVector &params)
{
    _params = params;
    _primvarNames = _CollectPrimvarNames(_params);
    _isValidComputedHash = false;
}

void
HdStSurfaceShader::SetTextureDescriptors(const TextureDescriptorVector &texDesc)
{
    _textureDescriptors = texDesc;
    _isValidComputedHash = false;
}

void
HdStSurfaceShader::SetBufferSources(
    HdBufferSourceVector &bufferSources,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    if (bufferSources.empty()) {
        _paramArray.reset();
    } else {
        // Build the buffer Spec to see if its changed.
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(bufferSources, &bufferSpecs);

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
        }

        if (_paramArray->IsValid()) {
            resourceRegistry->AddSources(_paramArray, bufferSources);
        }
    }
    _isValidComputedHash = false;
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

static bool
operator== (HdStShaderCode::TextureDescriptor const & a,
            HdStShaderCode::TextureDescriptor const & b)
{
    return a.name == b.name &&
           a.handle == b.handle &&
           a.type == b.type;
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

    bool bindlessTexture = GlfContextCaps::GetInstance()
                                                .bindlessTextureEnabled;

    // Without bindless textures, we can't aggregate unless textures also match.
    if (!bindlessTexture) {
        bool texturesMatch = shaderA->GetTextures() == shaderB->GetTextures();
        if (!texturesMatch) {
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
_GetExtraWhitelistedShaderPrimvarNames()
{
    static const TfTokenVector primvarNames = {
        HdTokens->displayColor,
        HdTokens->displayOpacity,

        // Whitelist a few ad hoc primvar names that
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
_CollectPrimvarNames(const HdMaterialParamVector &params)
{
    TfTokenVector primvarNames = _GetExtraWhitelistedShaderPrimvarNames();

    for (HdMaterialParam const &param: params) {
        if (param.IsFallback()) {
            primvarNames.push_back(param.name);
        } else if (param.IsPrimvar()) {
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


PXR_NAMESPACE_CLOSE_SCOPE
