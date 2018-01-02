//
// Copyright 2017 Pixar
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

#include "pxr/imaging/hdSt/computeShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE



HdStComputeShader::HdStComputeShader()
 : HdShaderCode()
 , _computeSource()
 , _params()
 , _paramSpec()
 , _paramArray()
 , _textureDescriptors()
{
}

HdStComputeShader::~HdStComputeShader()
{
}

void
HdStComputeShader::_SetSource(TfToken const &shaderStageKey, std::string const &source)
{
    if (shaderStageKey == HdShaderTokens->computeShader) {
        _computeSource = source;
    }
}

// -------------------------------------------------------------------------- //
// HdShader Virtual Interface                                                 //
// -------------------------------------------------------------------------- //

/*virtual*/
std::string
HdStComputeShader::GetSource(TfToken const &shaderStageKey) const
{
    if (shaderStageKey == HdShaderTokens->computeShader) {
        return _computeSource;
    }

    return std::string();
}
/*virtual*/
HdMaterialParamVector const&
HdStComputeShader::GetParams() const
{
    return _params;
}
/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdStComputeShader::GetShaderData() const
{
    return _paramArray;
}
/*virtual*/
HdShaderCode::TextureDescriptorVector
HdStComputeShader::GetTextures() const
{
    return _textureDescriptors;
}
/*virtual*/
void
HdStComputeShader::BindResources(Hd_ResourceBinder const &binder, int program)
{
    binder.BindShaderResources(this);
}
/*virtual*/
void
HdStComputeShader::UnbindResources(Hd_ResourceBinder const &binder, int program)
{
    binder.UnbindShaderResources(this);
}
/*virtual*/
void
HdStComputeShader::AddBindings(HdBindingRequestVector *customBindings)
{
}

/*virtual*/
HdShaderCode::ID
HdStComputeShader::ComputeHash() const
{
    size_t hash = 0;
    
    TF_FOR_ALL(it, _params) {
        if (it->IsFallback())
            boost::hash_combine(hash, it->GetName().Hash());
    }
    boost::hash_combine(hash, 
        ArchHash(_computeSource.c_str(), _computeSource.size()));
    return hash;
}

void
HdStComputeShader::SetComputeSource(const std::string &source)
{
    _SetSource(HdShaderTokens->computeShader, source);
}

/// If the prim is based on asset, reload that asset.
void
HdStComputeShader::Reload()
{
    // Nothing to do, this shader's sources are externally managed.
}

PXR_NAMESPACE_CLOSE_SCOPE
