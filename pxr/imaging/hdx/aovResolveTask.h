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
#ifndef PXR_IMAGING_HDX_AOV_RESOLVE_TASK_H
#define PXR_IMAGING_HDX_AOV_RESOLVE_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdxAovResolveTask
///
/// A task that converts multi-sample texels into regular texels so that another
/// task/shader may read the texels. (MSAA resolve)
///
class HdxAovResolveTask : public HdTask {
public:
    HDX_API
    HdxAovResolveTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxAovResolveTask();

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Prepare the tasks resources
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;


    /// Execute render pass task
    HDX_API
    virtual void Execute(HdTaskContext* ctx);

private:
    HdxAovResolveTask()                                      = delete;
    HdxAovResolveTask(const HdxAovResolveTask &)             = delete;
    HdxAovResolveTask &operator =(const HdxAovResolveTask &) = delete;

    SdfPath _aovBufferPath;
    class HdRenderBuffer* _aovBuffer;
};


struct HdxAovResolveTaskParams
{
    HdxAovResolveTaskParams() {}

    SdfPath aovBufferPath;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxAovResolveTaskParams& pv);
HDX_API
bool operator==(const HdxAovResolveTaskParams& lhs,
                const HdxAovResolveTaskParams& rhs);
HDX_API
bool operator!=(const HdxAovResolveTaskParams& lhs,
                const HdxAovResolveTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
