//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extCompComputeShader.h"
#include "pxr/imaging/hdSt/extComputation.h"

#include "pxr/imaging/hdSt/binding.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/arch/hash.h"

#include "pxr/base/tf/hash.h"

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
                                         HdSt_ResourceBinder const &binder)
{
    // Compute shaders currently serve GPU ExtComputations, wherein
    // resource binding is managed explicitly.
    // See HdStExtCompGpuComputationResource::Resolve() and
    // HdStExtCompGpuComputation::Execute(..)
}

/*virtual*/
void
HdSt_ExtCompComputeShader::UnbindResources(const int program,
                                           HdSt_ResourceBinder const &binder)
{
    // Resource binding is managed explicitly. See above comment.
}

/*virtual*/
void
HdSt_ExtCompComputeShader::AddBindings(HdStBindingRequestVector *customBindings)
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
    hash = TfHash::Combine(hash, ArchHash(kernel.c_str(), kernel.size()));
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
