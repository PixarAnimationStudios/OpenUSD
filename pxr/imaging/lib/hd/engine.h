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
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/context.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hf/pluginDelegateDesc.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class GalDelegate;
class HdContext;
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
    HdEngine();
    virtual ~HdEngine();

    /// \name Hydra Plug-in Management
    ///
    ///
    /// @{

    /// Returns a list of registered Render Delegates for use
    /// for rendering the scene.  The list is ordered by priority
    /// then display-name alphabetical order.
    static void GetRenderDelegateDescs(HfPluginDelegateDescVector *delegates);

    /// Returns a list of registered Gal Delegates for use
    /// for scene display.  Typically only a single Gal plugin is used for
    /// an application.
    static void GetGalDelegateDescs(HfPluginDelegateDescVector *delegates);

    /// Set the default Render Delegate plugin to use for new hydra contexts
    void SetDefaultRenderDelegateId(const TfToken &renderDelegateId);

    /// Set the default Gal Delegate plugin to use for new hydra contexts
    void SetDefaultGalDelegateId(const TfToken &galDelegateId);

    const TfToken &GetDefaultRenderDelegateId();
    const TfToken &GetDefaultGalDelegateId();
    /// @}



    /// \name Context Management
    ///
    /// Hydra contexts allow an application to run multiple views of a
    /// scene in parallel.  For example, an application can create
    /// a context per window, or a context per parallel render task.
    ///
    /// A context can be active or inactive.  For efficiency, it is better
    /// to sync all contexts at once, but only those on the active list
    /// are synced, those on the inactive list are skipped.  Contexts start
    /// in the active state.
    ///
    /// @{

    HdContext *CreateContextWithDefaults();
    HdContext *CreateSharedContext(HdContext *srcContext);
    HdContext *CreateContext(const TfToken &renderDelegateId,
                             const TfToken &galDelegateId,
                             HdRenderIndex *index);

    void DestroyContext(HdContext *context);

    void ActivateContext(HdContext *context);
    void DeactivateContext(HdContext *context);

    /// @}

    /// \name Render Index
    ///
    /// The Render Index object is Hydra's primary scene management tool.
    /// Typically there would be one render index in the scene shared between
    /// contexts.
    ///
    /// Using the context creation functions, the application typically
    /// does not need to worry about managing the life-time of render index
    /// objects directly.  However, this API allow for more specific life-time
    /// management for applications that want finer control.
    ///
    /// @{

    static HdRenderIndex *CreateRenderIndex();
    static void AddRenderIndexReference(HdRenderIndex *renderIndex);
    static void ReleaseRenderIndex(HdRenderIndex *renderIndex);

    /// @}

    HdResourceRegistry& GetResourceRegistry() const
        { return *_resourceRegistry; }


    /// \name Task Context
    ///
    /// External interface to set data/state in the task context passed to
    /// each task in the render graph
    ///
    /// @{

    /// Adds or updates the value associated with the token.
    /// Only one is supported for each token.
    void SetTaskContextData(const TfToken &id, VtValue &data);

    /// Removes the specified token.
    void RemoveTaskContextData(const TfToken &id);

    /// @}

    /// Execute tasks.
    void Execute(HdRenderIndex& index, 
                 HdTaskSharedPtrVector const &tasks);

    /// (deprecated: use Execute instead) Draw a renderpass.
    void Draw(HdRenderIndex& index,
              HdRenderPassSharedPtr const &renderPass,
              HdRenderPassStateSharedPtr const &renderPassState);

    void ReloadAllShaders(HdRenderIndex& index);


private:
    // Used an initialized placeholder instead for the default delegates
    // instead of using an empty token as the empty token is valid.
    static const TfToken _UninitalizedId;

    // Store contexts in a intrusive list.
    // Don't use constant time size as we are going to use auto-unlink
    typedef boost::intrusive::list<HdContext,
                                   boost::intrusive::constant_time_size<false>
                                  > HdContextList;

    // Lists of contexts
    HdContextList _activeContexts;
    HdContextList _inactiveContexts;
    bool          _activeContextsDirty;

    // Default Delegates
    TfToken _defaultRenderDelegateId;
    TfToken _defaultGalDelegateId;

    // XXX FIX: To be moved to HdStream
    HdResourceRegistry* _resourceRegistry;

    /// XXX FIX: Move to HdContext.
    /// Context containing token-value pairs, that is passed to each
    /// task in the render graph.  The task-context can be pre-populated
    /// and managed externally, so the state is persistent between runs of the
    /// render graph.
    HdTaskContext _taskContext;

    void _InitCaps() const;
    HdContext *_CreateContext(HdRenderDelegate *renderDelegate,
                              GalDelegate      *galDelegate,
                              HdRenderIndex    *index);
    void _DeleteContext(HdContext *context);

    void _InitalizeDefaultRenderDelegateId();
    void _InitalizeDefaultGalDelegateId();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_ENGINE_H
