//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_TASK_H
#define PXR_IMAGING_HD_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/dictionary.h"

#include <memory>
#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


using HdTaskSharedPtr = std::shared_ptr<class HdTask>;
using HdTaskSharedPtrVector = std::vector<HdTaskSharedPtr>;

// We want to use token as a key not std::string, so use an unordered_map over
// VtDictionary
using HdTaskContext = 
    std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;

/// \class HdTask
///
/// HdTask represents a unit of work to perform during a Hydra render.
/// Developers can subclass HdTask to prepare resources, run 3d renderpasses, 
/// run 2d renderpasses such as compositing or color correction, or coordinate 
/// integration with the application or other renderers.
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
    /// (such as a camera prim) by querying the render index.
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

    /// Render Tag Gather.
    ///
    /// Is called during the Sync phase after the task has been sync'ed.
    ///
    /// The task should return the render tags it wants to be appended to the
    /// active set.
    ///
    /// Hydra prims are marked up with a render tag and only prims
    /// marked with the render tags in the current active set are Sync'ed.
    ///
    /// Hydra's core will combine the sets from each task and deduplicate the
    /// result.  So tasks don't need to co-ordinate with each other to
    /// optimize the set.
    ///
    /// For those tasks that use HdRenderPass, this set is passed
    /// to HdRenderPass's Execute method.
    ///
    /// The default implementation returns an empty set
    HD_API
    virtual const TfTokenVector &GetRenderTags() const;

    SdfPath const& GetId() const { return _id; }

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HD_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const;


protected:
    /// Check if the shared context contains a value for the given id.
    HD_API
    static bool _HasTaskContextData(HdTaskContext const* ctx,
                                    TfToken const& id);

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

    HD_API
    TfTokenVector _GetTaskRenderTags(HdSceneDelegate* delegate);

    /// Extract an object from a HdDriver inside the task context.
    /// Returns nullptr if driver was not found.
    template <class T>
    static T _GetDriver(
        HdTaskContext const* ctx,
        TfToken const& driverName);

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

template <class T>
T
HdTask::_GetDriver(
    HdTaskContext const* ctx,
    TfToken const& driverName)
{
    auto it = ctx->find(HdTokens->drivers);
    if (it != ctx->end()) {
        VtValue const& value = it->second;
        if (value.IsHolding<HdDriverVector>()) {
            HdDriverVector const& drivers= value.UncheckedGet<HdDriverVector>();
            for (HdDriver* hdDriver : drivers) {
                if (hdDriver->name == driverName) {
                    if (hdDriver->driver.IsHolding<T>()) {
                        return hdDriver->driver.UncheckedGet<T>();
                    }
                }
            }
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_TASK_H
