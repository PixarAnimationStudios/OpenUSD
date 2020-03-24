//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGIGL_PIPELINE_H
#define PXR_IMAGING_HGIGL_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/graphicsEncoderDesc.h"
#include "pxr/imaging/hgi/pipeline.h"

#include "pxr/imaging/hgiGL/api.h"

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiGLPipeline
///
/// OpenGL implementation of HgiPipeline.
///
class HgiGLPipeline final : public HgiPipeline
{
public:
    HGIGL_API
    HgiGLPipeline(HgiPipelineDesc const& desc);

    HGIGL_API
    virtual ~HgiGLPipeline();

    /// Apply pipeline state
    HGIGL_API
    void BindPipeline();

    /// CaptureOpenGlState and RestoreOpenGlState are transition helpers since
    /// not all rendering is going via Hgi yet.
    /// We restore all the old opengl state defensively assuming that
    /// non-hgi code is not careful with state management.
    /// However, this does not apply to newer api's like Vulkan or Metal where
    /// all state is reset to default at the end of a render pass.
    /// XXX Remove both these functions when Hgi transition is complete.
    HGIGL_API
    void CaptureOpenGlState();
    HGIGL_API
    void RestoreOpenGlState();

private:
    HgiGLPipeline() = delete;
    HgiGLPipeline & operator=(const HgiGLPipeline&) = delete;
    HgiGLPipeline(const HgiGLPipeline&) = delete;

private:
    HgiPipelineDesc _descriptor;

    int32_t _restoreDrawFramebuffer;
    int32_t _restoreReadFramebuffer;
    int32_t _restoreRenderBuffer;
    int32_t _restoreVao;
    bool _restoreDepthTest;
    bool _restoreDepthWriteMask;
    bool _restoreStencilWriteMask;
    int32_t _restoreDepthFunc;
    int32_t _restoreViewport[4];
    bool _restoreblendEnabled;
    int32_t _restoreColorOp;
    int32_t _restoreAlphaOp;
    int32_t _restoreColorSrcFnOp;
    int32_t _restoreAlphaSrcFnOp;
    int32_t _restoreColorDstFnOp;
    int32_t _restoreAlphaDstFnOp;
    bool _restoreAlphaToCoverage;

    uint32_t _vao;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
