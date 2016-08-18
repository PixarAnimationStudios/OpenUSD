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
#ifndef HDX_SELECTION_TASK_H
#define HDX_SELECTION_TASK_H

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/gf/vec4f.h"

#include <boost/shared_ptr.hpp>

class HdRenderIndex;
class HdSceneDelegate;

struct HdxSelectionTaskParams
{
    bool enableSelection;
    GfVec4f selectionColor;
    GfVec4f locateColor;
    GfVec4f maskColor;
};

/// \class HdxSelectionTask
///
/// The SelectionTask is responsible for setting up render pass global buffers
/// for selection and depositing those buffers into the task context for down
/// stream consumption. Any render pass which wants to display selection may
/// extract those buffers and bind them into the current render pass shader to
/// enable selection highlighting.
///
class HdxSelectionTask : public HdSceneTask {
public:
    HDXLIB_API
    HdxSelectionTask(HdSceneDelegate* delegate, SdfPath const& id);

protected:
    /// Execute render pass task
    virtual void _Execute(HdTaskContext* ctx);

    /// Sync the render pass resources
    virtual void _Sync(HdTaskContext* ctx);

private:
    int _lastVersion;
    int _offsetMin;
    int _offsetMax;
    bool _hasSelection;
     HdxSelectionTaskParams _params;
    HdBufferArrayRangeSharedPtr _selOffsetBar;
    HdBufferArrayRangeSharedPtr _selValueBar;
    HdBufferArrayRangeSharedPtr _selUniformBar;
};

// VtValue requirements
HDXLIB_API
std::ostream& operator<<(std::ostream& out,
                         const HdxSelectionTaskParams& pv);
HDXLIB_API
bool operator==(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs);
HDXLIB_API
bool operator!=(const HdxSelectionTaskParams& lhs,
                const HdxSelectionTaskParams& rhs);

#endif //HDX_SELECTION_TASK_H

