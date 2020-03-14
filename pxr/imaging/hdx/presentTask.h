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
#ifndef PXR_IMAGING_HDX_PRESENT_TASK_H
#define PXR_IMAGING_HDX_PRESENT_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/fullscreenShader.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxPresentTask
///
/// A task for taking output AOV data of a HdSt render buffer and rendering it 
/// to the current GL buffer.
///
class HdxPresentTask : public HdTask
{
public:
    HDX_API
    HdxPresentTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxPresentTask();

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Prepare the colorize task
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute the colorize task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    SdfPath _aovBufferPath;
    SdfPath _depthBufferPath;

    HdRenderBuffer *_aovBuffer;
    HdRenderBuffer *_depthBuffer;

    HdxFullscreenShader _compositor;

    HdxPresentTask() = delete;
    HdxPresentTask(const HdxPresentTask &) = delete;
    HdxPresentTask &operator =(const HdxPresentTask &) = delete;
};


/// \class HdxPresentTaskParams
///
/// PresentTask parameters.
///
struct HdxPresentTaskParams
{
    HdxPresentTaskParams()
        : aovBufferPath()
        , depthBufferPath()
        {}

    SdfPath aovBufferPath;
    SdfPath depthBufferPath;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxPresentTaskParams& pv);
HDX_API
bool operator==(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs);
HDX_API
bool operator!=(const HdxPresentTaskParams& lhs,
                const HdxPresentTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
