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

// We want to use token as a key not std::string, so use an unordered_map over
// VtDictionary
typedef std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>
                                                                  HdTaskContext;

class HdTask {
public:
    /// Construct a new task.
    /// If the task is going to be added to the render index, id
    /// should be an absolute scene path.
    /// If the task isn't going to be added to the render index
    /// an empty path should be used for id.
    HD_API
    HdTask(SdfPath const& id);

    HD_API
    virtual ~HdTask();

    /// Sync Phase:  Obtain task state from Scene delegate based on
    /// change processing.
    ///
    /// This function might only be called if dirtyBits is not 0,
    /// so isn't guaranteed to be called every time HdEngine::Execute() is run
    /// with this task.
    ///
    /// However, this is the only time when the task should communicate with
    /// with the scene delegate responsible for the task and should be
    /// used to pull all changed data.  As outside the Sync phase, the scene
    /// delegate may not have the data available.
    ///
    /// Tasks maybe synced in parallel and out of order.
    ///
    /// The ctx parameter is present for legacy reason and shouldn't be used
    /// once the task has moved to using the 3-phase mechanism.
    ///
    /// After a task has been synced, it is expected that it produces a
    /// collection identifying the prims that are important to the task.  This
    /// collection is used to filter the prims in the scene so only the
    /// Relevant prims get synced.
    ///
    /// Note about inter-prim dependencies:
    ///   Quite often tasks need to access other prims, such as a camera prim
    ///   for example.  These other prims have not been synced yet when sync is
    ///   called.  Therefore, it is not recommended to access these prims during
    ///   the sync phase.  Instead a task should store the path to the prim
    ///   to be resolved to an actual prim during the "prepare" phase.

    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) = 0;

    /// Prepare Phase: Resolve bindings and manage resources.
    ///
    /// The Prepare phase happens before the Data Commit phase.
    /// All tasks in the task list get called for every execute.
    /// At this time all Tasks and other prims have completed the phase synced.
    ///
    /// This is an opportunity for the task to pull data from other prims
    /// (such as a camera prim), but accessing the render index.
    ///
    /// The task can also use the phase to create, register and update temporary
    /// resources with the resource registry or other render delegate
    /// specific mechanism.
    ///
    /// Tasks are always "Prepared" in execution order.
    ///
    /// Inter-task communication is achievable via the task context.
    /// The same task context is used for the prepare and execution phases.
    /// Data in the task context isn't guaranteed to persist across calls
    /// to HdEngine::Execute().
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) = 0;

    /// Execute Phase: Runs the task.
    ///
    /// The execution phase should trigger render delegate processing,
    /// such as issuing draw commands.
    ///
    /// Task execution is non-parallel and ordered.
    ///
    /// The task context is the same as used by the prepare step and is used
    /// for inter-task communication.
    virtual void Execute(HdTaskContext* ctx) = 0;

    SdfPath const& GetId() const { return _id; }

protected:
    /// Extracts a typed value out of the task context at the given id.
    /// If the id is missing or of the wrong type, the code will
    /// throw a verify error, return false and outValue will be unmodified.
    /// in case of success, the return value is true and the value is
    /// copied into outValue.
    ///
    /// outValue must not be null.
    template <class T>
    static bool _GetTaskContextData(HdTaskContext const* ctx,
                                    TfToken const& id,
                                    T* outValue);

    /// Extracts a typed value out of the task context at the given id.
    /// If the id is missing or of the wrong type, the code will
    /// throw a verify error, return false and outValue will be unmodified.
    /// in case of success, the return value is true and the value is
    /// copied into outValue.
    ///
    /// outValue must not be null.
    template <class T>
    bool _GetTaskParams(HdSceneDelegate* delegate,
                        T* outValue);

private:
    SdfPath _id;

    HdTask()                           = delete;
    HdTask(const HdTask &)             = delete;
    HdTask &operator =(const HdTask &) = delete;
};

// Inline template body
template <class T>
bool
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
        TF_CODING_ERROR("Token %s in task context is of mismatched type",
                        id.GetText());
        return false;
    }

    *outValue = valueVt.UncheckedGet<T>();

    return true;
}

template <class T>
bool
HdTask::_GetTaskParams(HdSceneDelegate* delegate,
                       T*               outValue)
{
    TF_DEV_AXIOM(outValue != nullptr);

    SdfPath const& taskId = GetId();

    VtValue valueVt = delegate->Get(taskId, HdTokens->params);
    if (!valueVt.IsHolding<T>()) {
        TF_CODING_ERROR("Task params for %s is of unexpected type",
                        taskId.GetText());
        return false;
    }

    *outValue = valueVt.UncheckedGet<T>();

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_TASK_H
