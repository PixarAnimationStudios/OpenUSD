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
#ifndef PXR_IMAGING_HD_ST_MIXIN_SHADER_H
#define PXR_IMAGING_HD_ST_MIXIN_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderCode.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_MixinShader
///
/// A final shader code class representing a mixin of a shader with a 
/// base shader.
/// 
/// This interface provides a simple way for clients to extend a given
/// shader without mutating the original shader.
class HdStMixinShader final : public HdStShaderCode
{
public:

    HDST_API
    HdStMixinShader(
        std::string mixinSource,
        HdStShaderCodeSharedPtr baseShader);

    HDST_API
    ~HdStMixinShader() override;

    /// Returns the hash value of this shader.
    HDST_API
    HdStShaderCode::ID ComputeHash() const override;

    HDST_API
    ID ComputeTextureSourceHash() const override;

    /// Returns the shader source provided by this shader
    /// for \a shaderStageKey
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;

    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;

    HDST_API
    bool IsEnabledPrimvarFiltering() const override;

    HDST_API
    TfTokenVector const& GetPrimvarNames() const override;

    /// Returns a buffer which stores parameter fallback values and texture
    /// handles.
    HDST_API
    HdBufferArrayRangeSharedPtr const& GetShaderData() const override;

    /// Binds shader-specific resources to \a program
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder,
                       HdRenderPassState const &state) override;

    /// Unbinds shader-specific resources.
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder,
                         HdRenderPassState const &state) override;

    /// Add custom bindings (used by codegen)
    HDST_API
    void AddBindings(HdBindingRequestVector* customBindings) override;

    /// Returns the render pass tag of this shader.
    HDST_API
    TfToken GetMaterialTag() const override;

private:
    std::string _mixinSource;
    HdStShaderCodeSharedPtr _baseShader;
    
    HdStMixinShader() = delete;
    HdStMixinShader(const HdStMixinShader &) = delete;
    HdStMixinShader &operator =(const HdStMixinShader &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MIXIN_SHADER_H

