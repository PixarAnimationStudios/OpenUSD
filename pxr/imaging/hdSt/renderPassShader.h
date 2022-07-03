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
#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStRenderPassShaderSharedPtr =
    std::shared_ptr<class HdStRenderPassShader>;
class HdRenderIndex;
using HdRenderPassAovBindingVector =
    std::vector<struct HdRenderPassAovBinding>;

/// \class HdStRenderPassShader
///
/// A shader that supports common renderPass functionality.
///
class HdStRenderPassShader : public HdStShaderCode {
public:
    HDST_API
    HdStRenderPassShader();
    HDST_API
    HdStRenderPassShader(TfToken const &glslfxFile);
    HDST_API
    ~HdStRenderPassShader() override;

    /// HdShader overrides
    HDST_API
    ID ComputeHash() const override;
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;
    HDST_API
    void AddBindings(HdBindingRequestVector *customBindings) override;
    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;

    HDST_API
    NamedTextureHandleVector const & GetNamedTextureHandles() const override;

    /// Add a custom binding request for use when this shader executes.
    HDST_API
    void AddBufferBinding(HdBindingRequest const& req);

    /// Remove \p name from custom binding.
    HDST_API
    void RemoveBufferBinding(TfToken const &name);

    /// Clear all custom bindings associated with this shader.
    HDST_API
    void ClearBufferBindings();

    // Sets the textures and params such that the shader can access
    // the requested aovs with HdGet_AOVNAMEReadback().
    //
    // Needs to be called in task prepare or sync since it is
    // allocating texture handles.
    //
    HDST_API
    void UpdateAovInputTextures(
        HdRenderPassAovBindingVector const &aovInputBindings,
        HdRenderIndex * const renderIndex);

private:
    TfToken _glslfxFile;
    std::unique_ptr<HioGlslfx> _glslfx;
    mutable size_t  _hash;
    mutable bool    _hashValid;

    TfHashMap<TfToken, HdBindingRequest, TfToken::HashFunctor> _customBuffers;

    NamedTextureHandleVector _namedTextureHandles;

    HdSt_MaterialParamVector _params;

    // No copying
    HdStRenderPassShader(const HdStRenderPassShader &)                     = delete;
    HdStRenderPassShader &operator =(const HdStRenderPassShader &)         = delete;

    HioGlslfx const * _GetGlslfx() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H
