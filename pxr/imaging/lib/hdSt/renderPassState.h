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
#ifndef HDST_RENDER_PASS_STATE_H
#define HDST_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/renderPassState.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdResourceRegistry> HdResourceRegistrySharedPtr;
typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassState> HdStRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef boost::shared_ptr<class HdStLightingShader> HdStLightingShaderSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassShader>
                HdStRenderPassShaderSharedPtr;
typedef boost::shared_ptr<class HdSt_FallbackLightingShader>
                HdSt_FallbackLightingShaderSharedPtr;
typedef std::vector<HdStShaderCodeSharedPtr> HdStShaderCodeSharedPtrVector;

/// \class HdStRenderPassState
///
/// A set of rendering parameters used among render passes.
///
/// Parameters are expressed as GL states, uniforms or shaders.
///
class HdStRenderPassState : public HdRenderPassState {
public:
    HDST_API
    HdStRenderPassState();
    HDST_API
    HdStRenderPassState(HdStRenderPassShaderSharedPtr const &shader);
    HDST_API
    virtual ~HdStRenderPassState();

    HDST_API
    virtual void
    Prepare(HdResourceRegistrySharedPtr const &resourceRegistry) override;

    /// Apply the GL states.
    /// Following states may be changed and restored to
    /// the GL default at Unbind().
    ///   glEnable(GL_POLYGON_OFFSET_FILL)
    ///   glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
    ///   glEnable(GL_PROGRAM_POINT_SIZE);
    ///   glEnable(GL_STENCIL_TEST);
    ///   glPolygonOffset()
    ///   glDepthFunc()
    ///   glStencilFunc()
    ///   glStencilOp()
    ///   glLineWidth()
    HDST_API
    virtual void Bind() override;

    HDST_API
    virtual void Unbind() override;

    /// Set lighting shader
    HDST_API
    void SetLightingShader(HdStLightingShaderSharedPtr const &lightingShader);
    HdStLightingShaderSharedPtr const & GetLightingShader() const {
        return _lightingShader;
    }

    /// renderpass shader
    HDST_API
    void SetRenderPassShader(HdStRenderPassShaderSharedPtr const &renderPassShader);
    HdStRenderPassShaderSharedPtr const &GetRenderPassShader() const {
        return _renderPassShader;
    }

    /// override shader
    HDST_API
    void SetOverrideShader(HdStShaderCodeSharedPtr const &overrideShader);
    HdStShaderCodeSharedPtr const &GetOverrideShader() const {
        return _overrideShader;
    }

    /// returns shaders (lighting/renderpass)
    HDST_API
    HdStShaderCodeSharedPtrVector GetShaders() const;

    HDST_API
    size_t GetShaderHash() const;

private:
    // ---------------------------------------------------------------------- //
    // Shader Objects
    // ---------------------------------------------------------------------- //
    HdStRenderPassShaderSharedPtr _renderPassShader;
    HdSt_FallbackLightingShaderSharedPtr _fallbackLightingShader;
    HdStLightingShaderSharedPtr _lightingShader;
    HdStShaderCodeSharedPtr _overrideShader;

    HdBufferArrayRangeSharedPtr _renderPassStateBar;
    size_t _clipPlanesBufferSize;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_RENDER_PASS_STATE_H
