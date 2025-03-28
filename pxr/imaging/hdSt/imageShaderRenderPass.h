//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_IMAGE_SHADER_RENDER_PASS_H
#define PXR_IMAGING_HD_ST_IMAGE_SHADER_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hd/renderPass.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

using HdSt_ImageShaderRenderPassSharedPtr =
    std::shared_ptr<class HdSt_ImageShaderRenderPass>;
using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

class Hgi;

/// \class HdSt_ImageShaderRenderPass
///
/// A single, full-screen triangle render pass.
/// The task that creates this RenderPass should set a RenderPassShader on the
/// RenderPassState. The RenderPassShader is your full-screen post-effect.
/// The benefit of using RenderPassShader is that it participates in codegen.
/// This means your full-screen shader can use buffers created by other tasks.
///
class HdSt_ImageShaderRenderPass final : public HdRenderPass {
public:
    HDST_API
    HdSt_ImageShaderRenderPass(HdRenderIndex *index, 
                               HdRprimCollection const &collection);

    HDST_API
    virtual ~HdSt_ImageShaderRenderPass();

    // Set the vertex BAR and geometric shader for the triangle.
    // This function should be called before the Execute task phase.
    HDST_API
    void SetupFullscreenTriangleDrawItem();
protected:

    void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                  TfTokenVector const &renderTags) override;

private:
    // Setup a BAR for a single triangle
    void _SetupVertexPrimvarBAR(HdStResourceRegistrySharedPtr const&);

    // We re-use the immediateBatch to draw the full-screen triangle.
    HdRprimSharedData _sharedData;
    HdStDrawItem _drawItem;
    HdStDrawItemInstance _drawItemInstance;
    HdSt_DrawBatchSharedPtr _drawBatch;
    Hgi* _hgi;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
