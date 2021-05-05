//
// Copyright 2018 Pixar
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
#ifndef PXR_IMAGING_HDX_TASK_H
#define PXR_IMAGING_HDX_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;


/// \class HdxTask
///
/// Base class for (some) tasks in Hdx that provides common progressive 
/// rendering and Hgi functionality.
///
/// Tasks that require neither progressive rendering nor Hgi can continue to
/// derive directly from HdTask.
///
class HdxTask : public HdTask
{
public:
    HDX_API
    HdxTask(SdfPath const& id);

    HDX_API
    ~HdxTask() override;

    /// This function returns true when a (progressive) task considers its
    /// execution results converged. Usually this means that a progressive
    /// render delegate is finished rendering into the HdRenderBuffers used by
    /// this task.
    /// Returns true by default which is a good default for rasterizers.
    ///
    /// Applications with data-driven task lists can determine their convergence
    /// state by determining which tasks are HdxTasks and then querying
    /// specifically those tasks for IsConverged.
    HDX_API
    virtual bool IsConverged() const;

    /// We override HdTask::Sync, but make it 'final' to make sure derived
    /// classes can't override it and instead override _Sync.
    /// This 'non-virtual interface'-like pattern allows us to ensure we always
    /// initialized Hgi during the Sync task so derived classes don't have to.
    void Sync(
        HdSceneDelegate* delegate,
        HdTaskContext* ctx,
        HdDirtyBits* dirtyBits) final;

protected:
    // This is called during the hydra Sync Phase via HdxTask::Sync.
    // Please see HdTask::Sync for Sync Phase documentation.
    virtual void _Sync(
        HdSceneDelegate* delegate,
        HdTaskContext* ctx,
        HdDirtyBits* dirtyBits) = 0;

    // Swaps the color target and colorIntermediate target.
    // This is used when a task wishes to read from the color and also write
    // to it. We use two color targets and ping-pong between them.
    void _ToggleRenderTarget(HdTaskContext* ctx);

    // Return pointer to Hydra Graphics Interface.
    HDX_API
    Hgi* _GetHgi() const;

    Hgi* _hgi;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

