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
#ifndef PXR_IMAGING_HD_ST_COMPUTE_SHADER_H
#define PXR_IMAGING_HD_ST_COMPUTE_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_ComputeShaderSharedPtr = std::shared_ptr<class HdSt_ComputeShader>;

/// \class HdSt_ComputeShader
///
/// An internal representation of a compute shader in Storm that allows the
/// use of the code generation and resource binding system to generate a
/// shader program.
///
class HdSt_ComputeShader : public HdStShaderCode {
public:
    HDST_API
    HdSt_ComputeShader();
    HDST_API
    virtual ~HdSt_ComputeShader();


    // ---------------------------------------------------------------------- //
    /// \name HdStShaderCode (pure) virtual interface                         //
    // ---------------------------------------------------------------------- //

    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder,
                       HdRenderPassState const &state) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder,
                         HdRenderPassState const &state) override;
    HDST_API
    void AddBindings(HdBindingRequestVector *customBindings) override;
    HDST_API
    ID ComputeHash() const override;

    // ---------------------------------------------------------------------- //

    /// Setter method for prim
    HDST_API
    void SetComputeSource(const std::string &source);

private:
    std::string _computeSource;

    // No copying
    HdSt_ComputeShader(const HdSt_ComputeShader &) = delete;
    HdSt_ComputeShader &operator =(const HdSt_ComputeShader &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_COMPUTE_SHADER_H
