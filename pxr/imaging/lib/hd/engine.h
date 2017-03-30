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
#ifndef HD_ENGINE_H
#define HD_ENGINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/task.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdRenderDelegate;
class HdResourceRegistry;

typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;

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
    void SetTaskContextData(const TfToken &id, VtValue &data);

    /// Removes the specified token.
    HD_API
    void RemoveTaskContextData(const TfToken &id);

    /// @}

    /// Execute tasks.
    HD_API
    void Execute(HdRenderIndex& index, 
                 HdTaskSharedPtrVector const &tasks);

    /// (deprecated: use Execute instead) Draw a renderpass.
    HD_API
    void Draw(HdRenderIndex& index,
              HdRenderPassSharedPtr const &renderPass,
              HdRenderPassStateSharedPtr const &renderPassState);

    HD_API
    void ReloadAllShaders(HdRenderIndex& index);


private:
    /// Context containing token-value pairs, that is passed to each
    /// task in the render graph.  The task-context can be pre-populated
    /// and managed externally, so the state is persistent between runs of the
    /// render graph.
    HdTaskContext _taskContext;

    void _InitCaps() const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_ENGINE_H
