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
#ifndef HDX_DRAW_TARGET_RENDER_PASS_H
#define HDX_DRAW_TARGET_RENDER_PASS_H

#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/imaging/glf/drawTarget.h"

#include <vector>

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

class HdxDrawTargetRenderPassState;

/// \class HdxDrawTargetRenderPass
///
/// Represents an render pass that renders to a draw target.
///
/// \note This is a temporary API to aid transition to Hydra, and is subject
/// to major changes.  It is likely this functionality will be absorbed into
/// the base class.
///
class HdxDrawTargetRenderPass : boost::noncopyable {
public:
    HdxDrawTargetRenderPass(HdRenderIndex *index);
    virtual ~HdxDrawTargetRenderPass();

    /// Sets the target draw object of this render pass containing
    /// the color buffers and depth buffer to use.
    void SetDrawTarget(const GlfDrawTargetRefPtr &drawTarget);

    /// Returns the draw target associated to this render pass.
    GlfDrawTargetRefPtr GetDrawTarget();

    /// Sets the non-context dependent state.  The object is expected to
    /// live longer than this class.
    void SetRenderPassState(HdxDrawTargetRenderPassState *renderPassState);

    /// Sets the collection of rprims associated to this render pass.
    void SetRprimCollection(HdRprimCollection const& col);

    /// Execute render pass task
    void Sync();

    /// Sync the render pass resources
    void Execute(HdRenderPassStateSharedPtr const &renderPassState);

private:
    /// RenderPass
    HdRenderPass _renderPass;

    /// drawtarget renderPass state
    HdxDrawTargetRenderPassState *_drawTargetRenderPassState;

    /// Local copy of the draw target object.
    GlfDrawTargetRefPtr  _drawTarget;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;

    unsigned int         _collectionObjectVersion;

    /// Clear all color and depth buffers.
    void _ClearBuffers();

    // No default/copy
    HdxDrawTargetRenderPass()                                            = delete;
    HdxDrawTargetRenderPass(const HdxDrawTargetRenderPass &)             = delete;
    HdxDrawTargetRenderPass &operator =(const HdxDrawTargetRenderPass &) = delete;
};

#endif // HDX_DRAW_TARGET_RENDER_PASS_H
