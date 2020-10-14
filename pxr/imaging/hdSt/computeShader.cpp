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
#include "pxr/imaging/hdSt/computeShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/arch/hash.h"

PXR_NAMESPACE_OPEN_SCOPE



HdSt_ComputeShader::HdSt_ComputeShader()
 : HdStShaderCode()
{
}

HdSt_ComputeShader::~HdSt_ComputeShader()
{
}

// -------------------------------------------------------------------------- //
// HdStShaderCode Virtual Interface                                           //
// -------------------------------------------------------------------------- //

/*virtual*/
std::string
HdSt_ComputeShader::GetSource(TfToken const &shaderStageKey) const
{
    if (shaderStageKey == HdShaderTokens->computeShader) {
        return _computeSource;
    }

    return std::string();
}

/*virtual*/
void
HdSt_ComputeShader::BindResources(const int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state)
{
    // Compute shaders currently serve GPU ExtComputations, wherein
    // resource binding is managed explicitly.
    // See HdStExtCompGpuComputationResource::Resolve() and
    // HdStExtCompGpuComputation::Execute(..)
}

/*virtual*/
void
HdSt_ComputeShader::UnbindResources(const int program,
                                   HdSt_ResourceBinder const &binder,
                                   HdRenderPassState const &state)
{
    // Compute shaders currently serve GPU ExtComputations, wherein
    // resource binding is managed explicitly.
    // See HdStExtCompGpuComputationResource::Resolve() and
    // HdStExtCompGpuComputation::Execute(..)
}

/*virtual*/
void
HdSt_ComputeShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // Resource binding is managed explicitly. See above comment.
}

/*virtual*/
HdStShaderCode::ID
HdSt_ComputeShader::ComputeHash() const
{
    size_t hash = 0;
    boost::hash_combine(hash, 
        ArchHash(_computeSource.c_str(), _computeSource.size()));
    return hash;
}

// -------------------------------------------------------------------------- //

void
HdSt_ComputeShader::SetComputeSource(const std::string &source)
{
    _computeSource = source;
}

PXR_NAMESPACE_CLOSE_SCOPE
