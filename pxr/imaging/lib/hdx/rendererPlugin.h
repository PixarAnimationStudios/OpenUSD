//
// Copyright 2017 Pixar
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
#ifndef HDX_RENDERER_PLUGIN_H
#define HDX_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hf/pluginDelegateBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdRenderDelegate;
class HdRenderIndex;
class HdxTaskController;


///
/// This class defines a renderer plugin interface for Hydra.
/// A renderer plugin is a dynamically discovered and loaded at run-time using
/// the Plug system.
///
/// This object has singleton behavior, in that is instantiated once per
/// library (managed by the plugin registry).
///
/// The class is used to factory objects that provide delegate support
/// to other parts of the Hydra Ecosystem.
///
class HdxRendererPlugin : public HfPluginDelegateBase {
public:

    ///
    /// Factory a Render Delegate object, that Hydra can use to
    /// factory prims and communicate with a renderer.
    ///
    virtual HdRenderDelegate *CreateRenderDelegate() = 0;

    ///
    /// Release the object factoried by CreateRenderDelegate().
    ///
    virtual void DeleteRenderDelegate(HdRenderDelegate *renderDelegate) = 0;

    ///
    /// Create a Task controller object, which represents a task controller and
    /// a collection of tasks that can be used to generate output from
    /// Hydra.  This object allows a plugin to do basic operations without
    /// an application requiring prior-knowledge of the plugin.  However,
    /// it doesn't replace the need for that knowledge for doing more complex
    /// operations.
    ///
    /// In order to achieve these basic operations the Task Controller may
    /// add one of more tasks to the render index.  Multiple Task Controllers
    /// may be used on the same render index, as a result the tasks that are
    /// being added need a unique id.  The controllerId param is prepended onto
    /// a controller defined id in order to unquify these task names.
    ///
    /// \param renderIndex  The render index to use in conjunction with the
    ///                     tasks being managed by the controller.
    /// \param controllerId An path that is prefixed onto the ids the controller
    ///                     uses to create tasks(prims) in the render index in
    ///                     order to form a unique id.
    ///
    virtual HdxTaskController *CreateTaskController(
                                               HdRenderIndex *renderIndex,
                                               const SdfPath &controllerId) = 0;


    ///
    /// Release the object factoried by CreateTaskController().
    ///
    virtual void DeleteTaskController(HdxTaskController *taskController) = 0;

protected:
    HdxRendererPlugin() = default;
    HDX_API
    virtual ~HdxRendererPlugin();


private:
    // This class doesn't require copy support.
    HdxRendererPlugin(const HdxRendererPlugin &)             = delete;
    HdxRendererPlugin &operator =(const HdxRendererPlugin &) = delete;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDX_RENDERER_PLUGIN_H
