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
#ifndef PXR_IMAGING_HDX_COLORIZE_SELECTION_TASK_H
#define PXR_IMAGING_HDX_COLORIZE_SELECTION_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/compositor.h"
#include "pxr/imaging/hdx/progressiveTask.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxColorizeSelectionTaskParams
///
/// Input parameters for HdxColorizeSelectionTask
///
struct HdxColorizeSelectionTaskParams
{
    HdxColorizeSelectionTaskParams()
        : enableSelection(false)
        , selectionColor(0)
        , locateColor(0)
        , primIdBufferPath()
        , instanceIdBufferPath()
        , elementIdBufferPath()
        {}

    bool enableSelection;
    GfVec4f selectionColor;
    GfVec4f locateColor;

    SdfPath primIdBufferPath;
    SdfPath instanceIdBufferPath;
    SdfPath elementIdBufferPath;
};

/// \class HdxColorizeSelectionTask
///
/// A task for taking ID buffer data and turning it into a "selection overlay"
/// that can be composited on top of hydra's color output.
///
class HdxColorizeSelectionTask : public HdxProgressiveTask
{
public:
    HDX_API
    HdxColorizeSelectionTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxColorizeSelectionTask();

    /// Hooks for progressive rendering.
    virtual bool IsConverged() const override;

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Prepare the render pass resources
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute the task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    // The core colorizing logic of this task: given the ID buffers and the
    // selection buffer, produce a color output at each pixel.
    void _ColorizeSelection();

    GfVec4f _GetColorForMode(int mode) const;

    // Incoming data
    HdxColorizeSelectionTaskParams _params;

    int _lastVersion;
    bool _hasSelection;
    VtIntArray _selectionOffsets;

    HdRenderBuffer *_primId;
    HdRenderBuffer *_instanceId;
    HdRenderBuffer *_elementId;

    uint8_t *_outputBuffer;
    size_t _outputBufferSize;
    bool _converged;

    HdxCompositor _compositor;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out,
    const HdxColorizeSelectionTaskParams& pv);
HDX_API
bool operator==(const HdxColorizeSelectionTaskParams& lhs,
                const HdxColorizeSelectionTaskParams& rhs);
HDX_API
bool operator!=(const HdxColorizeSelectionTaskParams& lhs,
                const HdxColorizeSelectionTaskParams& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_COLORIZE_SELECTION_TASK_H
