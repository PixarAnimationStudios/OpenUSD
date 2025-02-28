//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_SELECTION_TASK_H
#define PXR_IMAGING_HDX_SELECTION_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec4f.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;
class HdSceneDelegate;

struct HdxSelectionTaskParams
{
    bool enableSelectionHighlight;
    bool enableLocateHighlight;
    float occludedSelectionOpacity; // lerp factor when blending 
                                    // occluded selection
    GfVec4f selectionColor; // "active" selection color
    GfVec4f locateColor; // "rollover" selection color
};

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

/// \class HdxSelectionTask
///
/// The SelectionTask is responsible for setting up render pass global buffers
/// for selection and depositing those buffers into the task context for down
/// stream consumption. Any render pass which wants to display selection may
/// extract those buffers and bind them into the current render pass shader to
/// enable selection highlighting.
///
class HdxSelectionTask : public HdTask
{
public:
    using TaskParams = HdxSelectionTaskParams;

    HDX_API
    HdxSelectionTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxSelectionTask() override;

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;
    

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;


private:
    int _lastVersion;
    bool _hasSelection;
    HdxSelectionTaskParams _params;
    HdBufferArrayRangeSharedPtr _selOffsetBar;
    HdBufferArrayRangeSharedPtr _selUniformBar;
    size_t _pointColorsBufferSize;

    HdxSelectionTask() = delete;
    HdxSelectionTask(const HdxSelectionTask &) = delete;
    HdxSelectionTask &operator =(const HdxSelectionTask &) = delete;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out,
                         const HdxSelectionTaskParams& pv);
HDX_API
bool operator==(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs);
HDX_API
bool operator!=(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_SELECTION_TASK_H

