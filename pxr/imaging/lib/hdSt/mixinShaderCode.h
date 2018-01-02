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
#ifndef HDST_MIXIN_SHADER_CODE_H
#define HDST_MIXIN_SHADER_CODE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderCode.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_MixinShaderCode
///
/// A final shader code class representing a mixin of a shader with a 
/// base shader.
/// 
/// This interface provides a simple way for clients to extend a given
/// shader without mutating the original shader.
class HdStMixinShaderCode final : public HdStShaderCode {
public:

    HDST_API
    HdStMixinShaderCode(std::string mixinSource,
                        HdStShaderCodeSharedPtr baseShader);

    HDST_API
    virtual ~HdStMixinShaderCode();

    /// Returns the hash value of this shader.
    virtual HdStShaderCode::ID ComputeHash() const override;

    /// Returns the shader source provided by this shader
    /// for \a shaderStageKey
    virtual std::string GetSource(TfToken const &shaderStageKey) const override;

    virtual HdMaterialParamVector const& GetParams() const override;

    virtual HdStShaderCode::TextureDescriptorVector GetTextures() const override;

    /// Returns a buffer which stores parameter fallback values and texture
    /// handles.
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const override;

    /// Binds shader-specific resources to \a program
    virtual void BindResources(HdSt_ResourceBinder const &binder,
                               int program) override;

    /// Unbinds shader-specific resources.
    virtual void UnbindResources(HdSt_ResourceBinder const &binder,
                                 int program) override;

    /// Add custom bindings (used by codegen)
    virtual void AddBindings(HdBindingRequestVector* customBindings) override;

private:
    std::string _mixinSource;
    HdStShaderCodeSharedPtr _baseShader;
    
    HdStMixinShaderCode()                                        = delete;
    HdStMixinShaderCode(const HdStMixinShaderCode &)             = delete;
    HdStMixinShaderCode &operator =(const HdStMixinShaderCode &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SHADER_H

