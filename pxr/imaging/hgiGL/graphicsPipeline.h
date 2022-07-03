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
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiGL/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiGL;

/// \class HgiGLPipeline
///
/// OpenGL implementation of HgiGraphicsPipeline.
///
class HgiGLGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
    HGIGL_API
    ~HgiGLGraphicsPipeline() override;

    /// Apply pipeline state
    HGIGL_API
    void BindPipeline();

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLGraphicsPipeline(HgiGL const* hgi,
                          HgiGraphicsPipelineDesc const& desc);

private:
    HgiGLGraphicsPipeline() = delete;
    HgiGLGraphicsPipeline & operator=(const HgiGLGraphicsPipeline&) = delete;
    HgiGLGraphicsPipeline(const HgiGLGraphicsPipeline&) = delete;

    HgiGL const *_hgi;
    uint32_t _vao;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
