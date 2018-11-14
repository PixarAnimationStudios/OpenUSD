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
#ifndef HD_TASK_H
#define HD_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/dictionary.h"

#include <vector>
#include <unordered_map>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdTask> HdTaskSharedPtr;
typedef std::vector<HdTaskSharedPtr> HdTaskSharedPtrVector;

typedef boost::shared_ptr<class HdSceneTask> HdSceneTaskSharedPtr;
typedef std::vector<HdSceneTaskSharedPtr> HdSceneTaskSharedPtrVector;

// We want to use token as a key not std::string, so use an unordered_map over VtDictionary
typedef std::unordered_map<TfToken, VtValue, TfToken::HashFunctor> HdTaskContext;

class HdTask {
public:
    HD_API
    HdTask();
    HD_API
    virtual ~HdTask();

    /// Sync the resources. Syncs this task, then child tasks, if applicable.
    HD_API
    void Sync(HdTaskContext* ctx);

    /// Execute the task. Runs this task, then child tasks, if applicable.
    HD_API
    void Execute(HdTaskContext* ctx);

protected:
    /// Extracts a typed value out of the task context at the given id.
    /// If the id is missing or of the wrong type, the code will
    /// throw a verify error, return false and outValue will be unmodified.
    /// in case of success, the return value is true and the value is
    /// copied into outValue.
    ///
    /// outValue must not be null.
    template <class T>
    static bool _GetTaskContextData(HdTaskContext const* ctx, TfToken const &id, T *outValue);


    // Protected versions of Sync and Execute are provided for derived classes
    // to override.
    HD_API
    virtual void _Sync( HdTaskContext* ctx) = 0;
    HD_API
    virtual void _Execute(HdTaskContext* ctx) = 0;

    // _MarkClean is a hook for when Sync() is done running.
    HD_API
    virtual void _MarkClean();
};

// Inline template body
template <class T> bool
HdTask::_GetTaskContextData(HdTaskContext const* ctx,
                            TfToken const& id,
                            T* outValue)
{
    TF_DEV_AXIOM(outValue != nullptr);

    if (!ctx) {
        return false;
    }

    HdTaskContext::const_iterator valueIt = ctx->find(id);
    if (valueIt == ctx->cend()) {
        TF_CODING_ERROR("Token %s missing from task context", id.GetText());
        return false;
    }

    const VtValue &valueVt = (valueIt->second);
    if (!valueVt.IsHolding<T>()) {
        TF_CODING_ERROR("Token %s in task context is of mismatched type", id.GetText());
        return false;
    }

    *outValue = valueVt.UncheckedGet<T>();

    return true;
}

///////////////////////////////////////////////////////////////////////////////


/// \class HdSceneTask
///
/// An HdTask that lives in the RenderIndex and is backed by a SceneDelegate.
/// The default sync
///
class HdSceneTask : public HdTask {
public:
    HD_API
    HdSceneTask(HdSceneDelegate* delegate, SdfPath const& id);

    SdfPath const&         GetId()       const { return _id; }
    HdSceneDelegate*       GetDelegate()       { return _delegate; }

protected:
    struct _TaskDirtyState
    {
        HdDirtyBits bits;
        int         collectionVersion;
    };

    HD_API
    virtual void _MarkClean();

    /// Obtains the set of dirty bits for the task.
    HD_API
    HdDirtyBits _GetTaskDirtyBits();

    /// Obtains the set of dirty bits of the task
    /// and also returns the current collection version number for the
    /// given collectionId.
    /// Both results are returned in the dirtyState parameter.
    ///
    /// dirtyState must not be null
    HD_API
    void _GetTaskDirtyState(TfToken const& collectionId, _TaskDirtyState *dirtyState);

    /// Extracts a typed value out of the task context at the given id.
    /// If the id is missing or of the wrong type, the code will
    /// throw a verify error, return false and outValue will be unmodified.
    /// in case of success, the return value is true and the value is
    /// copied into outValue.
    ///
    /// outValue must not be null.
    template <class T> bool _GetSceneDelegateValue(TfToken const&valueId, T* outValue);

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;
};

template <class T> bool
HdSceneTask::_GetSceneDelegateValue(TfToken const& valueId, T* outValue)
{
    TF_DEV_AXIOM(outValue != nullptr);

    SdfPath const& taskId = GetId();
    HdSceneDelegate* delegate = GetDelegate();

    VtValue valueVt = delegate->Get(taskId, valueId);
    if (!valueVt.IsHolding<T>()) {
        TF_CODING_ERROR("Token %s from scene delegate is of mismatched type", valueId.GetText());
        return false;
    }

    *outValue = valueVt.UncheckedGet<T>();

    return true;
}

// Task parameters for scene based synchronization
struct HdTaskParams {
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_TASK_H
