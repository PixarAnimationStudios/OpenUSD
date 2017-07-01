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
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //
// HdTask Definitions 
// -------------------------------------------------------------------------- //

HdTask::HdTask()
{
}

HdTask::~HdTask()
{
}

void
HdTask::Execute(HdTaskContext* ctx)
{
    _Execute(ctx);
    TF_FOR_ALL(task, _children) {
        (*task)->Execute(ctx);
    }
}

void
HdTask::Sync(HdTaskContext* ctx)
{
    _Sync(ctx);
    _SyncChildren(ctx, &_children);
    TF_FOR_ALL(task, _children) {
        (*task)->Sync(ctx);
    }
    _MarkClean();
}

void
HdTask::_MarkClean()
{
}

void
HdTask::_SyncChildren(HdTaskContext* ctx, HdTaskSharedPtrVector* children)
{
}

// -------------------------------------------------------------------------- //
// HdSceneTask Definitions 
// -------------------------------------------------------------------------- //

HdSceneTask::HdSceneTask(HdSceneDelegate* delegate, SdfPath const& id)
    : _delegate(delegate)
    , _id(id)
{
}

void
HdSceneTask::_MarkClean()
{
    // This may not be sufficient, for exmaple if we want to incrementally
    // clean dirty bits.
    _delegate->GetRenderIndex().GetChangeTracker().MarkTaskClean(_id);
}

void
HdSceneTask::_SyncChildren(HdTaskContext* ctx, HdTaskSharedPtrVector* children)
{
    // TODO: CYCLE DETECTION!
    //
    HdDirtyBits bits =
        _delegate->GetRenderIndex().GetChangeTracker().GetTaskDirtyBits(_id);

    if (bits & HdChangeTracker::DirtyChildren) {
        VtValue v = _delegate->Get(_id, HdTokens->children);
        if (!TF_VERIFY(v.IsHolding<SdfPathVector>())) {
            return;
        }

        SdfPathVector childIds = v.UncheckedGet<SdfPathVector>();

        children->clear();
        children->reserve(childIds.size());

        TF_FOR_ALL(i, childIds) {
            HdTaskSharedPtr const& task = _delegate->GetRenderIndex().GetTask(*i);
            if (!TF_VERIFY(task)) {
                continue;
            }
            children->push_back(task);
        }
    }
}

HdDirtyBits
HdSceneTask::_GetTaskDirtyBits()
{
    SdfPath const& id = GetId();
    HdSceneDelegate* delegate = GetDelegate();
    HdChangeTracker& changeTracker = delegate->GetRenderIndex().GetChangeTracker();

    return changeTracker.GetTaskDirtyBits(id);
}

void
HdSceneTask::_GetTaskDirtyState(TfToken const& collectionId, _TaskDirtyState* dirtyState)
{
    TF_DEV_AXIOM(dirtyState != nullptr);

    SdfPath const& id = GetId();
    HdSceneDelegate* delegate = GetDelegate();
    HdChangeTracker& changeTracker = delegate->GetRenderIndex().GetChangeTracker();

    dirtyState->bits              = changeTracker.GetTaskDirtyBits(id);
    dirtyState->collectionVersion = changeTracker.GetCollectionVersion(collectionId);
}

PXR_NAMESPACE_CLOSE_SCOPE

