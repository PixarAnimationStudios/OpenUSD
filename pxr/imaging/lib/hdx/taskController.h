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
#ifndef HDX_TASK_CONTROLLER_H
#define HDX_TASK_CONTROLLER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: This API is transitional.
// Eventually, camera and lighting should be managed as Sprims, and the
// render/picking/selection APIs could be decoupled.

/// Task set tokens:
/// - "colorRender" is the set of tasks needed to render to a color buffer.
/// - "idRender" is the set of tasks needed to render an id buffer, indicating
///              what object is at each pixel.
#define HDX_TASK_SET_TOKENS                    \
    (colorRender)                              \
    (idRender)

TF_DECLARE_PUBLIC_TOKENS(HdxTaskSetTokens, HDX_API, HDX_TASK_SET_TOKENS);

/// Intersection mode tokens, mapped to HdxIntersector API.
/// Note: "nearest" hitmode may be considerably more efficient.
/// - "nearest" returns the nearest single hit point.
/// - "unique"  returns the set of unique hit prims, keeping only the nearest
///             depth per prim.
/// - "all"     returns all hit points, possibly including multiple hits per
///             prim.
#define HDX_INTERSECTION_MODE_TOKENS           \
    (nearest)                                  \
    (unique)                                   \
    (all)

TF_DECLARE_PUBLIC_TOKENS(HdxIntersectionModeTokens, HDX_API, \
    HDX_INTERSECTION_MODE_TOKENS);

class HdxTaskController {
public:
    HDX_API
    virtual ~HdxTaskController();

    /// Return the render index this controller is bound to.
    HdRenderIndex* GetRenderIndex() { return _index; }
    HdRenderIndex const* GetRenderIndex() const { return _index; }

    /// Return the controller's scene-graph id (prefixed to any
    /// scene graph objects it creates).
    SdfPath const& GetControllerId() { return _controllerId; }

    /// -------------------------------------------------------
    /// Execution API

    /// Obtain the set of tasks managed by the task controller
    /// suitable for execution. Currently supported tasksets:
    /// HdxTaskSet->render
    /// HdxTaskSet->idRender
    ///
    /// A vector of zero length indicates the specified taskSet is unsupported.
    virtual HdTaskSharedPtrVector const &GetTasks(TfToken const& taskSet) = 0;

    /// -------------------------------------------------------
    /// Rendering API

    /// Set the collection to be rendered.
    virtual void SetCollection(HdRprimCollection const& collection) = 0;

    /// Set the render params. Note: params.camera and params.viewport will
    /// be overwritten, since they come from SetCameraState.
    virtual void SetRenderParams(HdxRenderTaskParams const& params) = 0;

    /// -------------------------------------------------------
    /// Lighting API

    /// Set the lighting state for the scene.
    /// @param src    Lighting state to implement.
    /// @param bypass Toggle whether we use HdxSimpleLightTask,
    ///               or HdxSimpleLightBypassTask.  The former stores lighting
    ///               state in Sprims.
    /// XXX: remove "bypass"
    virtual void SetLightingState(GlfSimpleLightingContextPtr const& src,
                                  bool bypass) = 0;

    /// -------------------------------------------------------
    /// Camera API
    
    /// Set the parameters for the viewer default camera.
    virtual void SetCameraMatrices(GfMatrix4d const& viewMatrix,
                                   GfMatrix4d const& projectionMatrix) = 0;

    /// Set the camera viewport.
    virtual void SetCameraViewport(GfVec4d const& viewport) = 0;

    /// Set the camera clip planes.
    virtual void SetCameraClipPlanes(
            std::vector<GfVec4d> const& clipPlanes) = 0;

    /// -------------------------------------------------------
    /// Picking API

    /// Set pick target resolution (if applicable).
    /// XXX: Is there a better place for this to live? This is stream-specific.
    virtual void SetPickResolution(unsigned int size) = 0;

    /// Test for intersection.
    virtual bool TestIntersection(
            HdEngine* engine,
            HdRprimCollection const& collection,
            HdxIntersector::Params const& qparams,
            TfToken const& intersectionMode,
            HdxIntersector::HitVector *allHits) = 0;

    /// -------------------------------------------------------
    /// Selection API

    /// Turns the selection task on or off.
    virtual void SetEnableSelection(bool enable) = 0;

    /// Set the selection color.
    virtual void SetSelectionColor(GfVec4f const& color) = 0;

    /// -------------------------------------------------------
    /// Progressive Image Generation
    
    /// Reset the image render to reflect a changed scene.
    virtual void ResetImage() = 0;

    /// Return whether the image has converged.
    virtual bool IsConverged() const = 0;

protected:
    /// This class must be derived from
    HdxTaskController(HdRenderIndex *renderIndex,
                      SdfPath const& controllerId)
        : _index(renderIndex), _controllerId(controllerId)
        {}

private:
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(HdxTaskController const&) = delete;
    HdxTaskController &operator=(HdxTaskController const&) = delete;

    HdRenderIndex *_index;
    SdfPath const _controllerId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_TASK_CONTROLLER_H
