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
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/usd/sdf/path.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRequest.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectionContext.h>
#include <maya/MTypes.h>
#include <maya/MUserData.h>

#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEBUG_CODES(
    PXRUSDMAYAGL_QUEUE_INFO,
    PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING
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
/// AddShapeAdapter() to add their shape for batched drawing and selection.
///
/// In preparation for drawing, the shape adapter should be synchronized to
/// populate it with data from its shape and from the viewport display state.
/// A batch draw data object should also be created for the shape by calling
/// CreateBatchDrawData().
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

    /// Get the render index owned by the batch renderer.
    ///
    /// Clients of the batch renderer should use this render index to construct
    /// their delegates.
    PXRUSDMAYAGL_API
    HdRenderIndex* GetRenderIndex() const;

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

    /// Create the batch draw data for drawing in the legacy viewport.
    ///
    /// This draw data is attached to the given \p drawRequest. Its lifetime is
    /// *not* managed by Maya, so it must be deleted manually at the end of a
    /// legacy viewport Draw().
    ///
    /// \p boxToDraw may be set to nullptr if no box is desired to be drawn.
    ///
    PXRUSDMAYAGL_API
    void CreateBatchDrawData(
            MPxSurfaceShapeUI* shapeUI,
            MDrawRequest& drawRequest,
            const PxrMayaHdRenderParams& params,
            const bool drawShape,
            const MBoundingBox* boxToDraw = nullptr);

    /// Create the batch draw data for drawing in Viewport 2.0.
    ///
    /// \p oldData should be the same \p oldData parameter that Maya passed
    /// into the calling prepareForDraw() method. The return value from this
    /// method should then be returned back to Maya in the calling
    /// prepareForDraw().
    ///
    /// Note that this version of CreateBatchDrawData() is also invoked by the
    /// legacy viewport version, in which case we expect oldData to be nullptr.
    ///
    /// \p boxToDraw may be set to nullptr if no box is desired to be drawn.
    ///
    /// Returns a pointer to a new MUserData object populated with the given
    /// parameters if oldData is nullptr, otherwise returns oldData after
    /// having re-populated it.
    PXRUSDMAYAGL_API
    MUserData* CreateBatchDrawData(
            MUserData* oldData,
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

    /// Tests the object from the given shape adapter for intersection with
    /// a given view using the legacy viewport.
    ///
    /// \p hitPoint yields the point of intersection if \c true is returned.
    ///
    PXRUSDMAYAGL_API
    bool TestIntersection(
            const PxrMayaHdShapeAdapter* shapeAdapter,
            M3dView& view,
            const bool singleSelection,
            GfVec3f* hitPoint);

    /// Tests the object from the given shape adapter for intersection with
    /// a given draw context in Viewport 2.0.
    ///
    /// \p hitPoint yields the point of intersection if \c true is returned.
    ///
    PXRUSDMAYAGL_API
    bool TestIntersection(
            const PxrMayaHdShapeAdapter* shapeAdapter,
            const MHWRender::MSelectionInfo& selectInfo,
            const MHWRender::MDrawContext& context,
            const bool singleSelection,
            GfVec3f* hitPoint);

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

    /// Call to render all queued batches. May be called safely without
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
    /// legacy viewport. In that case, vp2Context will be nullptr.
    void _MayaRenderDidEnd(const MHWRender::MDrawContext* vp2Context);

    /// Update the last render frame stamp using the given \p frameStamp.
    ///
    /// Note that frame stamps are only available from the MDrawContext when
    /// using Viewport 2.0.
    ///
    /// Returns true if the last frame stamp was updated, or false if the given
    /// frame stamp is the same as the last frame stamp.
    bool _UpdateRenderFrameStamp(const MUint64 frameStamp);

    /// Update the last selection frame stamp using the given \p frameStamp.
    ///
    /// Note that frame stamps are only available from the MDrawContext when
    /// using Viewport 2.0.
    ///
    /// Returns true if the last frame stamp was updated, or false if the given
    /// frame stamp is the same as the last frame stamp.
    bool _UpdateSelectionFrameStamp(const MUint64 frameStamp);

    /// Update the internal marker of whether a legacy viewport render is
    /// pending.
    ///
    /// Returns true if the internal marker's value was changed, or false if
    /// the given value is the same as the current value.
    bool _UpdateLegacyRenderPending(const bool isPending);

    /// Update the internal marker of whether a legacy viewport selection is
    /// pending.
    ///
    /// Returns true if the internal marker's value was changed, or false if
    /// the given value is the same as the current value.
    bool _UpdateLegacySelectionPending(const bool isPending);

    /// With Viewport 2.0, we can query the draw context for its frameStamp,
    /// a pseudo-unique identifier for each draw/select operation. We use that
    /// to determine when to do a batched draw or batched selection versus when
    /// to simply pass through or re-use cached data.
    ///
    /// The legacy viewport however does not provide a context we can query for
    /// the frameStamp, so we simulate it with bools instead. The first call to
    /// CreateBatchDrawData() indicates that we are prepping for a render, so
    /// we know then that a render is pending. Rendering invalidates selection,
    /// so when a render completes, we mark selection as pending.
    MUint64 _lastRenderFrameStamp;
    MUint64 _lastSelectionFrameStamp;
    bool _legacyRenderPending;
    bool _legacySelectionPending;

    /// Type definition for a set of pointers to shape adapters.
    typedef std::unordered_set<PxrMayaHdShapeAdapter*> _ShapeAdapterSet;

    /// A shape adapter bucket is a pairing of a render params object and a
    /// set of shape adapters that all share those render params. Shape
    /// adapters are gathered together this way to minimize Hydra/OpenGL state
    /// changes when performing batched draws/selections.
    typedef std::pair<PxrMayaHdRenderParams, _ShapeAdapterSet> _ShapeAdapterBucket;

    /// This is the batch renderer's primary container for storing the current
    /// bucketing of all shape adapters registered with the batch renderer.
    /// The map is indexed by the hash of the bucket's render params object.
    typedef std::unordered_map<size_t, _ShapeAdapterBucket> _ShapeAdapterBucketsMap;

    /// We maintain separate bucket maps for Viewport 2.0 and the legacy
    /// viewport.
    _ShapeAdapterBucketsMap _shapeAdapterBuckets;

    _ShapeAdapterBucketsMap _legacyShapeAdapterBuckets;

    /// We detect and store whether Viewport 2.0 is using the legacy
    /// viewport-based selection mechanism (i.e. whether the
    /// MAYA_VP2_USE_VP1_SELECTION environment variable is enabled) when the
    /// batch renderer is constructed. Then when a legacy selection is
    /// performed, we consult this value and the viewport renderer of the
    /// M3dView in which the selection is occurring to determine which bucket
    /// map of shape adapters we should use to compute the selection.
    bool _viewport2UsesLegacySelection;

    /// Populates the selection results using the given parameters by
    /// performing intersection tests against all of the shapes in the given
    /// \p bucketsMap.
    void _ComputeSelection(
            _ShapeAdapterBucketsMap& bucketsMap,
            const GfMatrix4d& viewMatrix,
            const GfMatrix4d& projectionMatrix,
            const bool singleSelection);

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
    HdxSelectionTrackerSharedPtr _selectionTracker;

    UsdMayaGLSoftSelectHelper _softSelectHelper;
};

PXRUSDMAYAGL_API_TEMPLATE_CLASS(TfSingleton<UsdMayaGLBatchRenderer>);


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_BATCH_RENDERER_H
