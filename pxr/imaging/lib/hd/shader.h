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
#ifndef HD_SHADER_H
#define HD_SHADER_H

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/resourceBinder.h"  // XXX: including a private class
#include "pxr/imaging/hd/shaderParam.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

typedef std::vector<class HdBindingRequest> HdBindingRequestVector;

typedef boost::shared_ptr<class HdShader> HdShaderSharedPtr;
typedef boost::shared_ptr<class HdTexture> HdTextureSharedPtr;
typedef std::vector<HdShaderSharedPtr> HdShaderSharedPtrVector;
typedef std::vector<HdShaderSharedPtr> HdTextureSharedPtrVector;

/// \class HdShader
///
/// A shader base class, used in conjunction with HdRenderPass.
///
/// This interface provides a simple way for clients to affect the
/// composition of shading programs used for a render pass.
class HdShader {
public:
    typedef size_t ID;

    HdShader();
    virtual ~HdShader();

    /// Returns the hash value of this shader.
    virtual ID ComputeHash() const = 0;

    /// Returns the combined hash values of multiple shaders.
    static ID ComputeHash(HdShaderSharedPtrVector const &shaders);

    /// Returns the shader source provided by this shader
    /// for \a shaderStageKey
    virtual std::string GetSource(TfToken const &shaderStageKey) const = 0;

    // XXX: Should be pure-virtual
    /// Returns the shader parameters for this shader.
    virtual HdShaderParamVector const& GetParams() const;

    struct TextureDescriptor {
        TfToken name;
        size_t handle; // GLuint64, for bindless textures
        enum { TEXTURE_2D, TEXTURE_PTEX_TEXEL, TEXTURE_PTEX_LAYOUT };
        int type;
        unsigned int sampler;
    };
    typedef std::vector<TextureDescriptor> TextureDescriptorVector;

    // XXX: DOC
    virtual TextureDescriptorVector GetTextures() const;

    // XXX: Should be pure-virtual
    /// Returns a buffer which stores parameter fallback values and texture
    /// handles.
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;

    /// Binds shader-specific resources to \a program
    /// XXX: this interface is meant to be used for bridging
    /// the GlfSimpleLightingContext mechanism, and not for generic use-cases.
    virtual void BindResources(Hd_ResourceBinder const &binder,
                               int program) = 0;

    /// Unbinds shader-specific resources.
    virtual void UnbindResources(Hd_ResourceBinder const &binder,
                                 int program) = 0;

    /// Add custom bindings (used by codegen)
    virtual void AddBindings(HdBindingRequestVector* customBindings) = 0;
};

#endif //HD_SHADER_H
