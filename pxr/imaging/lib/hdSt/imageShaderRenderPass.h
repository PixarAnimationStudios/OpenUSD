//
// Copyright 2019 Pixar
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
#ifndef HDST_IMAGE_SHADER_RENDER_PASS_H
#define HDST_IMAGE_SHADER_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hd/renderPass.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStResourceRegistry>
    HdStResourceRegistrySharedPtr;
typedef boost::shared_ptr<class HdSt_ImageShaderRenderPass> 
    HdSt_ImageShaderRenderPassSharedPtr;
typedef boost::shared_ptr<class HdSt_DrawBatch> 
    HdSt_DrawBatchSharedPtr;

/// \class HdSt_ImageShaderRenderPass
///
/// A single, full-screen triangle render pass.
/// The task that creates this RenderPass should set a RenderPassShader on the
/// RenderPassState. The RenderPassShader is your full-screen post-effect.
/// The benefit of using RenderPassShader is that it participates in codegen.
/// This means your full-screen shader can use buffers created by other tasks.
///
class HdSt_ImageShaderRenderPass : public HdRenderPass {
public:
    HDST_API
    HdSt_ImageShaderRenderPass(HdRenderIndex *index, 
                               HdRprimCollection const &collection);

    HDST_API
    virtual ~HdSt_ImageShaderRenderPass();

protected:
    virtual void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                          TfTokenVector const &renderTags) override;

    virtual void _MarkCollectionDirty() override;

private:
    // Setup a BAR for a single triangle
    void _SetupVertexPrimvarBAR(HdStResourceRegistrySharedPtr const&);

    // We re-use the immediateBatch to draw the full-screen triangle.
    HdRprimSharedData _sharedData;
    HdStDrawItem _drawItem;
    HdStDrawItemInstance _drawItemInstance;
    HdSt_DrawBatchSharedPtr _immediateBatch;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
