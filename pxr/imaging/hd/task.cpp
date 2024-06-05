//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //
// HdTask Definitions 
// -------------------------------------------------------------------------- //

HdTask::HdTask(SdfPath const& id)
 : _id(id)
{
}

HdTask::~HdTask()
{
}

const TfTokenVector &
HdTask::GetRenderTags() const
{
    static TfTokenVector EMPTY_SET;

    return EMPTY_SET;
}

/// Returns the minimal set of dirty bits to place in the
/// change tracker for use in the first sync of this prim.
/// Typically this would be all dirty bits.
HdDirtyBits
HdTask::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::DirtyParams     |
           HdChangeTracker::DirtyCollection |
           HdChangeTracker::DirtyRenderTags;
}

bool
HdTask::_HasTaskContextData(
    HdTaskContext const* ctx,
    TfToken const& id)
{
    HdTaskContext::const_iterator valueIt = ctx->find(id);
    return (valueIt != ctx->cend());
}

TfTokenVector HdTask::_GetTaskRenderTags(HdSceneDelegate* delegate)
{
    return delegate->GetTaskRenderTags(GetId());
}

PXR_NAMESPACE_CLOSE_SCOPE

