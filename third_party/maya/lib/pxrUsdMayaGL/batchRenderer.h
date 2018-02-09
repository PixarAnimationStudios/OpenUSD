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
#ifndef PXRUSDMAYAGL_BATCH_RENDERER_H
#define PXRUSDMAYAGL_BATCH_RENDERER_H

/// \file batchRenderer.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/sceneDelegate.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/usd/sdf/path.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRequest.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MUserData.h>

#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEBUG_CODES(
    PXRUSDMAYAGL_QUEUE_INFO
);


/// UsdMayaGLBatchRenderer is a singleton that shapes can use to get consistent
/// batched drawing via Hydra in Maya, regardless of legacy viewport or
/// Viewport 2.0 usage.
///
/// Typical usage is as follows:
///
/// Objects that manage drawing and selection of Maya shapes (e.g. classes
/// derived from \c MPxSurfaceShapeUI or \c MPxDrawOverride) should construct
/// and maintain a PxrMayaHdShapeAdapter. Those objects should call
/// AddShapeAdapter() to add their shape for batched drawing and selection. The
/// batch renderer will pass along its \c HdRenderIndex to the shape adapter to
/// initialize it if the shape adapter had not previously been added.
///
/// At every refresh, in the prepare for draw stage, the shape adapter should
/// be populated with Maya scene graph and viewport display data. It should
/// then be passed along to QueueShapeForDraw() for every batch draw pass
/// desired.
///
/// In the draw stage, Draw() must be called for each draw request to complete
/// the render.
///
/// Draw/selection management objects should be sure to call
/// RemoveShapeAdapter() (usually in the destructor) when they no longer wish
/// for their shape to participate in batched drawing and selection.
///
class UsdMayaGLBatchRenderer : public TfSingleton<UsdMayaGLBatchRenderer>
{
public:

    /// Initialize the batch renderer.
    ///
    /// This should be called at least once and it is OK to call it multiple
    /// times. This handles things like initializing OpenGL/Glew.
    PXRUSDMAYAGL_API
    static void Init();

    /// Get the singleton instance of the batch renderer.
    PXRUSDMAYAGL_API
    static UsdMayaGLBatchRenderer& GetInstance();

    /// Add the given shape adapter for batched rendering and selection.
    ///
    /// Returns true if the shape adapter had not been previously added, or
    /// false otherwise.
    PXRUSDMAYAGL_API
    bool AddShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter);

    /// Remove the given shape adapter from batched rendering and selection.
    ///
    /// Returns true if the shape adapter was removed from internal caches, or
    /// false otherwise.
    PXRUSDMAYAGL_API
    bool RemoveShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter);

    /// \brief Queue a batch draw call, to be executed later.
    ///
    /// \p boxToDraw may be set to nullptr if no box is desired to be drawn.
    ///
    PXRUSDMAYAGL_API
    void QueueShapeForDraw(
            PxrMayaHdShapeAdapter* shapeAdapter,
            MPxSurfaceShapeUI* shapeUI,
            MDrawRequest& drawRequest,
            const PxrMayaHdRenderParams& params,
            const bool drawShape,
            const MBoundingBox* boxToDraw = nullptr);

    /// \brief Queue a batch draw call, to be executed later.
    ///
    /// \p userData should be the same parameter \c oldData passed into the
    /// caller: the overridden \c prepareForDraw(...) call. The \p userData
    /// pointer must also be returned from the overridden caller.
    ///
    /// \p boxToDraw may be set to nullptr if no box is desired to be drawn.
    ///
    PXRUSDMAYAGL_API
    void QueueShapeForDraw(
            PxrMayaHdShapeAdapter* shapeAdapter,
            MUserData*& userData,
            const PxrMayaHdRenderParams& params,
            const bool drawShape,
            const MBoundingBox* boxToDraw = nullptr);

    /// \brief Reset the internal state of the global UsdMayaGLBatchRenderer.
    /// In particular, it's important that this happen when switching to a new
    /// Maya scene so that any UsdImagingDelegates held by shape adapters that
    /// have been populated with USD stages can have those stages released,
    /// since the delegates hold a strong pointer to their stages.
    PXRUSDMAYAGL_API
    static void Reset();

    /// \brief Render batch or bounds in VP1 based on \p request
    PXRUSDMAYAGL_API
    void Draw(const MDrawRequest& request, M3dView& view);

    /// \brief Render batch or bounds in VP2 based on \p userData
    PXRUSDMAYAGL_API
    void Draw(
            const MHWRender::MDrawContext& context,
            const MUserData* userData);

    /// \brief Tests the object from the given shape renderer for intersection
    /// with a given view.
    ///
    /// \p hitPoint yields the point of intersection if \c true is returned.
    ///
    PXRUSDMAYAGL_API
    bool TestIntersection(
            const PxrMayaHdShapeAdapter* shapeAdapter,
            M3dView& view,
            const unsigned int pickResolution,
            const bool singleSelection,
            GfVec3d* hitPoint);

private:

    friend class TfSingleton<UsdMayaGLBatchRenderer>;

    PXRUSDMAYAGL_API
    UsdMayaGLBatchRenderer();

    PXRUSDMAYAGL_API
    virtual ~UsdMayaGLBatchRenderer();

    /// Gets the UsdMayaGLSoftSelectHelper that this batchRenderer maintains.
    ///
    /// This should only be used by PxrMayaHdShapeAdapter.
    PXRUSDMAYAGL_API
    const UsdMayaGLSoftSelectHelper& GetSoftSelectHelper();

    /// Allow shape adapters access to the soft selection helper.
    friend PxrMayaHdShapeAdapter;

    /// Private helper function for registering a batch render call.
    void _QueueShapeForDraw(
            PxrMayaHdShapeAdapter* shapeAdapter,
            const PxrMayaHdRenderParams& params);

    /// \brief Tests an object for intersection with a given view.
    ///
    /// \returns Hydra Hit info for instance associated with \p delegateId
    ///
    const HdxIntersector::Hit* _GetHitInfo(
            M3dView& view,
            const unsigned int pickResolution,
            const bool singleSelection,
            const SdfPath& delegateId,
            const GfMatrix4d& localToWorldSpace);

     /// \brief Call to render all queued batches. May be called safely w/o
    /// performance hit when no batches are queued.
    void _RenderBatches(
            const MHWRender::MDrawContext* vp2Context,
            const GfMatrix4d& worldToViewMatrix,
            const GfMatrix4d& projectionMatrix,
            const GfVec4d& viewport);

    /// \brief Handler for Maya Viewport 2.0 end render notifications.
    ///
    /// Viewport 2.0 may execute a render in multiple passes (shadow, color,
    /// etc.), and Maya sends a notification when all rendering has finished.
    /// When this notification is received, this method is invoked to reset
    /// some state in the batch renderer and prepare it for subsequent
    /// selection.
    /// For the legacy viewport, there is no such notification sent by Maya.
    static void _OnMayaEndRenderCallback(
            MHWRender::MDrawContext& context,
            void* clientData);

    /// \brief Perform post-render state cleanup.
    ///
    /// For Viewport 2.0, this method gets invoked by
    /// _OnMayaEndRenderCallback() and is what does the actual cleanup work.
    /// For the legacy viewport, there is no such notification sent by Maya, so
    /// this method is called internally at the end of Hydra draws for the
    /// legacy viewport.
    void _MayaRenderDidEnd();

    /// Cache of pointers to shape adapters registered with the batch renderer.
    typedef std::unordered_set<PxrMayaHdShapeAdapter*> _ShapeAdapterSet;

    _ShapeAdapterSet _shapeAdapterSet;

    /// Associative pair of PxrMayaHdRenderParams and shape adapter sets of
    /// shapes to be rendered with said params.
    typedef std::pair<PxrMayaHdRenderParams, _ShapeAdapterSet> _RenderParamSet;

    /// Lookup table to to find _RenderParamSet given a param hash key.
    typedef std::unordered_map<size_t, _RenderParamSet> _RendererQueueMap;

    /// Container of all batched render calls to be made at next display
    /// refresh.
    _RendererQueueMap _renderQueue;

    /// Container of batched render calls made at last display refresh, to be
    /// used at next selection operation.
    _RendererQueueMap _selectQueue;

    /// Container of Maya render pass identifiers of passes drawn so far during
    /// a Viewport 2.0 render.
    ///
    /// Since all Hydra geometry is drawn at once, we only ever want to execute
    /// the Hydra draw once per Maya render pass (shadow, color, etc.). This
    /// container keeps track of which passes have been drawn by Hydra, and it
    /// is reset when the batch renderer is notified that a Maya render has
    /// ended.
    std::unordered_set<std::string> _drawnMayaRenderPasses;

    typedef std::unordered_map<SdfPath, HdxIntersector::Hit, SdfPath::Hash> HitBatch;

    /// \brief a cache of all selection results gathered since the last display
    /// refresh.
    HitBatch _selectResults;

    /// \brief Hydra engine objects used to render batches.
    /// Note that the Hydra render index is constructed with and is dependent
    /// on the render delegate. At destruction time, the render index uses the
    /// delegate to destroy Hydra prims, so the delegate must be destructed
    /// *after* the render index. We enforce that ordering by declaring the
    /// render delegate *before* the render index, since class members are
    /// destructed in reverse declaration order.
    HdEngine _hdEngine;
    HdStRenderDelegate _renderDelegate;
    std::unique_ptr<HdRenderIndex> _renderIndex;

    PxrMayaHdSceneDelegateSharedPtr _taskDelegate;
    std::unique_ptr<HdxIntersector> _intersector;
    UsdMayaGLSoftSelectHelper _softSelectHelper;
};

PXRUSDMAYAGL_API_TEMPLATE_CLASS(TfSingleton<UsdMayaGLBatchRenderer>);


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_BATCH_RENDERER_H
