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
//

#include "pxr/imaging/hdSt/mixinShaderCode.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStMixinShaderCode::HdStMixinShaderCode(std::string mixinSource,
                                         HdShaderCodeSharedPtr baseShader)
: HdShaderCode()
, _mixinSource(mixinSource)
, _baseShader(baseShader)
{
}

HdStMixinShaderCode::~HdStMixinShaderCode()
{
}

HdShaderCode::ID HdStMixinShaderCode::ComputeHash() const 
{
    HdShaderCode::ID hash = 0;
    boost::hash_combine(hash, ArchHash(_mixinSource.c_str(), _mixinSource.size()));
    boost::hash_combine(hash, _baseShader->ComputeHash());
    return hash;
}

std::string HdStMixinShaderCode::GetSource(TfToken const &shaderStageKey) const 
{
    std::string baseSource = _baseShader->GetSource(shaderStageKey);
    if (shaderStageKey == HdShaderTokens->fragmentShader) {
        return _mixinSource + baseSource;
    }
    return baseSource;
}

HdMaterialParamVector const& HdStMixinShaderCode::GetParams() const 
{
    return _baseShader->GetParams();
}

HdBufferArrayRangeSharedPtr const& HdStMixinShaderCode::GetShaderData() const 
{
    return _baseShader->GetShaderData();
}

HdShaderCode::TextureDescriptorVector HdStMixinShaderCode::GetTextures() const 
{
    return _baseShader->GetTextures();
}

void HdStMixinShaderCode::BindResources(Hd_ResourceBinder const &binder,
                                        int program) 
{
    _baseShader->BindResources(binder, program);
}

void HdStMixinShaderCode::UnbindResources(Hd_ResourceBinder const &binder,
                                          int program)
{
    _baseShader->UnbindResources(binder, program);
}

void HdStMixinShaderCode::AddBindings(HdBindingRequestVector *customBindings)
{
    _baseShader->AddBindings(customBindings);
}

PXR_NAMESPACE_CLOSE_SCOPE


