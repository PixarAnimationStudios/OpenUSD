//
// Copyright 2022 Pixar
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
#ifndef HDX_BOUNDINGBOX_TASK_H
#define HDX_BOUNDINGBOX_TASK_H

#include "pxr/pxr.h"

#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/camera.h"

#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStRenderPassState;

/// \class HdxBoundingBoxTaskParams
///
/// BoundingBoxTask parameters.
///
struct HdxBoundingBoxTaskParams
{
    using BBoxVector = std::vector<GfBBox3d>;

    HDX_API
    HdxBoundingBoxTaskParams()
        : aovName()
        , bboxes()
        , color(1)
        , dashSize(3)
        {}

    TfToken aovName;

    // Data provided by the application
    BBoxVector bboxes;
    GfVec4f color;
    float dashSize;
};

/// \class HdxBoundingBoxTask
///
/// A task for rendering bounding boxes.
///
class HdxBoundingBoxTask : public HdxTask
{
public:
    HDX_API
    HdxBoundingBoxTask(HdSceneDelegate* delegate, const SdfPath& id);

    HDX_API
    ~HdxBoundingBoxTask() override;

    /// Prepare the bounding box task resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute the bounding box task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxBoundingBoxTask() = delete;
    HdxBoundingBoxTask(const HdxBoundingBoxTask&) = delete;
    HdxBoundingBoxTask &operator =(const HdxBoundingBoxTask&) = delete;

    // Utility function to create the shader for drawing dashed lines.
    bool _CreateShaderResources();

    // Utility function to create buffer resources.
    bool _CreateBufferResources();

    // Utility to create resource bindings
    bool _CreateResourceBindings();

    // Utility to create a pipeline.
    bool _CreatePipeline(
        const HgiTextureHandle& colorTexture,
        const HgiTextureHandle& depthTexture);

    // Utility to get the view and projection matrix from the camera.
    GfMatrix4d _ComputeViewProjectionMatrix(
        const HdStRenderPassState& hdStRenderPassState) const;

    // Utility to set the shader constants for drawing.
    void _UpdateShaderConstants(
        HgiGraphicsCmds* gfxCmds,
        const GfVec4i& gfxViewport,
        const HdStRenderPassState& hdStRenderPassState);

    // Create and submit the draw commands.
    void _DrawBBoxes(
        const HgiTextureHandle& colorTexture,
        const HgiTextureHandle& depthTexture,
        const HdStRenderPassState& hdStRenderPassState);

    // Destroy shader program and the shader functions it holds.
    void _DestroyShaderProgram();

    // Print shader compile errors.
    void _PrintCompileErrors();

    HgiAttachmentDesc _colorAttachment;
    HgiAttachmentDesc _depthAttachment;

    HgiBufferHandle _vertexBuffer;
    size_t _maxTransforms;
    HgiBufferHandle _transformsBuffer;
    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsPipelineHandle _pipeline;

    HdxBoundingBoxTaskParams _params;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxBoundingBoxTaskParams& pv);
HDX_API
bool operator==(const HdxBoundingBoxTaskParams& lhs,
                const HdxBoundingBoxTaskParams& rhs);
HDX_API
bool operator!=(const HdxBoundingBoxTaskParams& lhs,
                const HdxBoundingBoxTaskParams& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
