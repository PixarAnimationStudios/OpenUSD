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
#ifndef HD_DRAW_TARGET_RENDER_PASS_H
#define HD_DRAW_TARGET_RENDER_PASS_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/simpleLightingShader.h"

#include "pxr/imaging/glf/drawTarget.h"

#include <vector>

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

class HdDrawTargetRenderPassState;

/// Represents an render pass that renders to a draw target
/// XXX::  This is a temporary api to aid transition to Hydra.
/// and subject to major changes.  It is likely this functionality
/// will be absorbed into the base class.
class HdDrawTargetRenderPass : public HdTask {
public:
	HDLIB_API
    HdDrawTargetRenderPass(HdRenderIndex *index);
	HDLIB_API
    virtual ~HdDrawTargetRenderPass();

    /// Sets the target draw object of this render pass containing
    /// the color buffers and depth buffer to use.
	HDLIB_API
    void SetDrawTarget(const GlfDrawTargetRefPtr &drawTarget);

    /// Sets the non-context dependent state.  The object is expected to
    /// live longer than this class.
	HDLIB_API
    void SetRenderPassState(HdDrawTargetRenderPassState *renderPassState);

	HDLIB_API
    void SetRprimCollection(HdRprimCollection const& col);

    HdRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }

protected:
    /// Execute render pass task
    virtual void _Sync(HdTaskContext* ctx)    override;

    /// Sync the render pass resources
    virtual void _Execute(HdTaskContext* ctx) override;

private:
    /// RenderPass and state
    HdRenderPass _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;

    /// drawtarget renderPass state
    HdDrawTargetRenderPassState *_drawTargetRenderPassState;

    /// Local copy of the draw target object.
    GlfDrawTargetRefPtr  _drawTarget;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;

    HdSimpleLightingShaderSharedPtr _simpleLightingShader;
    GfMatrix4d           _viewMatrix;
    GfMatrix4d           _projectionMatrix;
    
    unsigned int         _collectionObjectVersion;

    /// Prepares the lighting context for this specific draw target pass
    void _UpdateLightingContext(GlfSimpleLightingContextRefPtr &lightingContext);

    /// Clear all color and depth buffers.
    void _ClearBuffers();

    // No default/copy
    HdDrawTargetRenderPass()                                           = delete;
    HdDrawTargetRenderPass(const HdDrawTargetRenderPass &)             = delete;
    HdDrawTargetRenderPass &operator =(const HdDrawTargetRenderPass &) = delete;
};

#endif // HD_DRAW_TARGET_RENDER_PASS_H
