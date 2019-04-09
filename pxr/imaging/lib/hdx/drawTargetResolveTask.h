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
#ifndef HDX_DRAW_TARGET_TASK_RESOLVE_H
#define HDX_DRAW_TARGET_TASK_RESOLVE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdxDrawTargetResolveTask  : public HdTask {
public:
    HDX_API
    HdxDrawTargetResolveTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxDrawTargetResolveTask();

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

    HdxDrawTargetResolveTask()                                      = delete;
    HdxDrawTargetResolveTask(const HdxDrawTargetResolveTask &)      = delete;
    HdxDrawTargetResolveTask &operator =(const HdxDrawTargetResolveTask &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_DRAW_TARGET_RESOLVE_TASK_H
