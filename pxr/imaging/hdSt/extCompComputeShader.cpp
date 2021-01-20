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
#include "pxr/imaging/hdSt/extCompComputeShader.h"
#include "pxr/imaging/hdSt/extComputation.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/arch/hash.h"

PXR_NAMESPACE_OPEN_SCOPE



HdSt_ExtCompComputeShader::HdSt_ExtCompComputeShader(
    HdExtComputation const *extComp)
 : _extComp(extComp)
{
}

HdSt_ExtCompComputeShader::~HdSt_ExtCompComputeShader() = default;

// -------------------------------------------------------------------------- //
// HdStShaderCode Virtual Interface                                           //
// -------------------------------------------------------------------------- //

/*virtual*/
std::string
HdSt_ExtCompComputeShader::GetSource(TfToken const &shaderStageKey) const
{
    if (shaderStageKey == HdShaderTokens->computeShader) {
         if (TF_VERIFY(_extComp)) {
            return _extComp->GetGpuKernelSource();
         }
    }

    return std::string();
}

/*virtual*/
void
HdSt_ExtCompComputeShader::BindResources(const int program,
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
HdSt_ExtCompComputeShader::UnbindResources(const int program,
                                   HdSt_ResourceBinder const &binder,
                                   HdRenderPassState const &state)
{
    // Resource binding is managed explicitly. See above comment.
}

/*virtual*/
void
HdSt_ExtCompComputeShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // Resource binding is managed explicitly. See above comment.
}

/*virtual*/
HdStShaderCode::ID
HdSt_ExtCompComputeShader::ComputeHash() const
{
    if (!TF_VERIFY(_extComp)) {
        return 0;
    }

    size_t hash = 0;
    std::string const & kernel = _extComp->GetGpuKernelSource();
    boost::hash_combine(hash, ArchHash(kernel.c_str(), kernel.size()));
    return hash;
}

SdfPath const&
HdSt_ExtCompComputeShader::GetExtComputationId() const
{
    if (!TF_VERIFY(_extComp)) {
        return SdfPath::EmptyPath();
    }
    return _extComp->GetId();
}

PXR_NAMESPACE_CLOSE_SCOPE
