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
#ifndef HD_RENDER_PASS_SHADER_H
#define HD_RENDER_PASS_SHADER_H

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class HdRenderPassShader> HdRenderPassShaderSharedPtr;

/// A shader that supports common renderPass functionality
///
class HdRenderPassShader : public HdShader {
public:
    HdRenderPassShader();
    HdRenderPassShader(TfToken const &glslfxFile);
    virtual ~HdRenderPassShader() override;

    /// HdShader overrides
    virtual ID ComputeHash() const override;
    virtual std::string GetSource(TfToken const &shaderStageKey) const override;
    virtual void BindResources(Hd_ResourceBinder const &binder, int program) override;
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program) override;
    virtual void AddBindings(HdBindingRequestVector *customBindings) override;

    /// Add a custom binding request for use when this shader executes.
    void AddBufferBinding(HdBindingRequest const& req);

    /// Remove \p name from custom binding.
    void RemoveBufferBinding(TfToken const &name);

    /// Clear all custom bindings associated with this shader.
    void ClearBufferBindings();

    HdCullStyle GetCullStyle() const {
        return _cullStyle;
    }

    void SetCullStyle(HdCullStyle cullStyle) {
        _cullStyle = cullStyle;
    }

private:
    TfToken _glslfxFile;
    boost::scoped_ptr<GlfGLSLFX> _glslfx;
    mutable size_t  _hash;
    mutable bool    _hashValid;

    TfHashMap<TfToken, HdBindingRequest, TfToken::HashFunctor> _customBuffers;
    HdCullStyle _cullStyle;
};

#endif // HD_RENDER_PASS_SHADER_H
