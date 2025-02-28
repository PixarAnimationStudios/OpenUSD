//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ENGINE_H
#define PXR_IMAGING_HD_ENGINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/task.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdRenderDelegate;
class HdResourceRegistry;

using HdRenderPassSharedPtr = std::shared_ptr<class HdRenderPass>;
using HdRenderPassStateSharedPtr = std::shared_ptr<class HdRenderPassState>;

/// \class HdEngine
///
/// The application-facing entry point top-level entry point for accessing Hydra.
/// Typically the application would only create one of these.
class HdEngine {
public:
    HD_API
    HdEngine();
    HD_API
    virtual ~HdEngine();

    /// \name Task Context
    ///
    /// External interface to set data/state in the task context passed to
    /// each task in the render graph
    ///
    /// @{

    /// Adds or updates the value associated with the token.
    /// Only one is supported for each token.
    HD_API
    void SetTaskContextData(const TfToken &id, const VtValue &data);

    /// If found, will return the value from the task context data associated
    /// with the token. Returns false if the data could not be found.
    HD_API
    bool GetTaskContextData(const TfToken &id, VtValue *data) const;

    /// Removes the specified token.
    HD_API
    void RemoveTaskContextData(const TfToken &id);

    /// Removes all keys.
    HD_API
    void ClearTaskContextData();

    /// @}

    /// Execute tasks.
    HD_API
    void Execute(HdRenderIndex *index,
                 HdTaskSharedPtrVector *tasks);


    HD_API
    void Execute(HdRenderIndex * index,
                 const SdfPathVector &taskPaths);

private:
    /// Context containing token-value pairs, that is passed to each
    /// task in the render graph.  The task-context can be pre-populated
    /// and managed externally, so the state is persistent between runs of the
    /// render graph.
    HdTaskContext _taskContext;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ENGINE_H
