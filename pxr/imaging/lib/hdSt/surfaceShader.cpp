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

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE



HdStSurfaceShader::HdStSurfaceShader()
 : HdShaderCode()
 , _fragmentSource()
 , _geometrySource()
 , _params()
 , _paramSpec()
 , _paramArray()
 , _textureDescriptors()
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
    } else if (shaderStageKey == HdShaderTokens->geometryShader) {
        _geometrySource = source;
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
HdShaderParamVector const&
HdStSurfaceShader::GetParams() const
{
    return _params;
}
/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdStSurfaceShader::GetShaderData() const
{
    return _paramArray;
}
/*virtual*/
HdShaderCode::TextureDescriptorVector
HdStSurfaceShader::GetTextures() const
{
    return _textureDescriptors;
}
/*virtual*/
void
HdStSurfaceShader::BindResources(Hd_ResourceBinder const &binder, int program)
{
    // XXX: there's an issue where other shaders try to use textures.
    int samplerUnit = binder.GetNumReservedTextureUnits();
    TF_FOR_ALL(it, _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it->name);
        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, (GLuint)it->handle);
            glBindSampler(samplerUnit, it->sampler);
            
            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, (GLuint)it->handle);

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, (GLuint)it->handle);

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        }
    }
    glActiveTexture(GL_TEXTURE0);
    binder.BindShaderResources(this);
}
/*virtual*/
void
HdStSurfaceShader::UnbindResources(Hd_ResourceBinder const &binder, int program)
{
    binder.UnbindShaderResources(this);

    int samplerUnit = binder.GetNumReservedTextureUnits();
    TF_FOR_ALL(it, _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it->name);
        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindSampler(samplerUnit, 0);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
            samplerUnit++;
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
HdShaderCode::ID
HdStSurfaceShader::ComputeHash() const
{
    size_t hash = 0;
    
    TF_FOR_ALL(it, _params) {
        if (it->IsFallback())
            boost::hash_combine(hash, it->GetName().Hash());
    }
    boost::hash_combine(hash, 
        ArchHash(_fragmentSource.c_str(), _fragmentSource.size()));
    boost::hash_combine(hash, 
        ArchHash(_geometrySource.c_str(), _geometrySource.size()));
    return hash;
}

void
HdStSurfaceShader::SetFragmentSource(const std::string &source)
{
    _fragmentSource = source;
}

void
HdStSurfaceShader::SetGeometrySource(const std::string &source)
{
    _geometrySource = source;
}

void
HdStSurfaceShader::SetParams(const HdShaderParamVector &params)
{
    _params = params;
}

void
HdStSurfaceShader::SetTextureDescriptors(const TextureDescriptorVector &texDesc)
{
    _textureDescriptors = texDesc;
}

void
HdStSurfaceShader::SetBufferSources(HdBufferSourceVector &bufferSources,
                                  HdResourceRegistrySharedPtr const &resourceRegistry)
{
    if (bufferSources.empty()) {
        _paramArray.reset();
    } else {
        // Build the buffer Spec to see if its changed.
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(srcIt, bufferSources) {
            (*srcIt)->AddBufferSpecs(&bufferSpecs);
        }

        if (!_paramArray || _paramSpec != bufferSpecs) {
            _paramSpec = bufferSpecs;

            // establish a buffer range
            HdBufferArrayRangeSharedPtr range =
                    resourceRegistry->AllocateShaderStorageBufferArrayRange(
                                                  HdTokens->surfaceShaderParams,
                                                  bufferSpecs);

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
}

/// If the prim is based on asset, reload that asset.
void
HdStSurfaceShader::Reload()
{
    // Nothing to do, this shader's sources are externally managed.
}

PXR_NAMESPACE_CLOSE_SCOPE
