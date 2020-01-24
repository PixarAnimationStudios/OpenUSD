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
#ifndef PXR_IMAGING_HD_ST_SHADER_CODE_H
#define PXR_IMAGING_HD_ST_SHADER_CODE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/materialParam.h"
#include "pxr/imaging/hdSt/resourceBinder.h"  // XXX: including a private class
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::vector<class HdBindingRequest> HdBindingRequestVector;

typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef std::vector<HdStShaderCodeSharedPtr> HdStShaderCodeSharedPtrVector;

typedef boost::shared_ptr<class HdStTextureResourceHandle>
                HdStTextureResourceHandleSharedPtr;

class HdRenderPassState;


/// \class HdStShaderCode
///
/// A base class representing the implementation (code) of a shader,
/// used in conjunction with HdRenderPass.
///
/// This interface provides a simple way for clients to affect the
/// composition of shading programs used for a render pass.
class HdStShaderCode {
public:
    typedef size_t ID;

    HDST_API
    HdStShaderCode();
    HDST_API
    virtual ~HdStShaderCode();

    /// Returns the hash value of this shader.
    virtual ID ComputeHash() const = 0;

    /// Returns the combined hash values of multiple shaders.
    HDST_API
    static ID ComputeHash(HdStShaderCodeSharedPtrVector const &shaders);

    /// Returns the shader source provided by this shader
    /// for \a shaderStageKey
    virtual std::string GetSource(TfToken const &shaderStageKey) const = 0;

    // XXX: Should be pure-virtual
    /// Returns the shader parameters for this shader.
    HDST_API
    virtual HdMaterialParamVector const& GetParams() const;

    /// Returns whether primvar filtering is enabled for this shader.
    HDST_API
    virtual bool IsEnabledPrimvarFiltering() const;

    /// Returns the names of primvar that are used by this shader.
    HDST_API
    virtual TfTokenVector const& GetPrimvarNames() const;

    struct TextureDescriptor {
        TfToken name;
        HdStTextureResourceHandleSharedPtr handle;
        enum { TEXTURE_2D, TEXTURE_3D, TEXTURE_UDIM_ARRAY, TEXTURE_UDIM_LAYOUT,
               TEXTURE_PTEX_TEXEL, TEXTURE_PTEX_LAYOUT };
        int type;
    };
    typedef std::vector<TextureDescriptor> TextureDescriptorVector;

    // XXX: DOC
    HDST_API
    virtual TextureDescriptorVector GetTextures() const;

    // XXX: Should be pure-virtual
    /// Returns a buffer which stores parameter fallback values and texture
    /// handles.
    HDST_API
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;

    /// Binds shader-specific resources to \a program
    /// XXX: this interface is meant to be used for bridging
    /// the GlfSimpleLightingContext mechanism, and not for generic use-cases.
    virtual void BindResources(int program,
                               HdSt_ResourceBinder const &binder,
                               HdRenderPassState const &state) = 0;

    /// Unbinds shader-specific resources.
    virtual void UnbindResources(int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state) = 0;

    /// Add custom bindings (used by codegen)
    virtual void AddBindings(HdBindingRequestVector* customBindings) = 0;

    /// Material tags can be set in the meta-data of a glslfx file to control
    /// what rprim collection that prims using this shader should go into.
    /// E.g. We can use it to split opaque and translucent prims into different
    /// collections. When no material tags are specified in the shader, a empty
    /// token is returned.
    HDST_API
    virtual TfToken GetMaterialTag() const;

private:

    // No copying
    HdStShaderCode(const HdStShaderCode &)                      = delete;
    HdStShaderCode &operator =(const HdStShaderCode &)          = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_SHADER_H
