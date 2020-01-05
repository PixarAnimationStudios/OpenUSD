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

/// \file pxrUsdMayaGL/batchRenderer.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/sceneDelegate.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "pxrUsdMayaGL/softSelectHelper.h"
#include "usdMaya/diagnosticDelegate.h"
#include "usdMaya/notice.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/usd/sdf/path.h"

#include <maya/M3dView.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRequest.h>
#include <maya/MObjectHandle.h>
#include <maya/MMessage.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionContext.h>
#include <maya/MTypes.h>
#include <maya/MUserData.h>

#include <memory>
#include <utility>
#include <unordered_map>
#include <unordered_set>


PXR_NAMESPACE_OPEN_SCOPE


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
/// A user data object should also be created/obtained for the shape by calling
/// the shape adapter's GetMayaUserData() method.
///
/// In the draw stage, Draw() must be called for each draw request to complete
/// the render.
///
/// Draw/selection management objects should be sure to call
/// RemoveShapeAdapter() (usually in the destructor) when they no longer wish
/// for their shape to participate in batched drawing and selection.
///
class UsdMayaGLBatchRenderer
        : public TfSingleton<UsdMayaGLBatchRenderer>, public TfWeakBase
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

    /// Get the delegate ID prefix for the specified viewport.
    ///
    /// The batch renderer has a root SdfPath under which it maintains separate
    /// hierarchies for shape adapter delegates based on whether they are for
    /// the legacy viewport or for Viewport 2.0. Shape adapters should use this
    /// method to request the appropriate prefix from the batch renderer when
    /// building the ID for their delegate.
    PXRUSDMAYAGL_API
    SdfPath GetDelegatePrefix(const bool isViewport2) const;

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

    /// Reset the internal state of the global UsdMayaGLBatchRenderer.
    ///
    /// In particular, it's important that this happen when switching to a new
    /// Maya scene so that any UsdImagingDelegates held by shape adapters that
    /// have been populated with USD stages can have those stages released,
    /// since the delegates hold a strong pointer to their stages.
    PXRUSDMAYAGL_API
    static void Reset();

    /// Replaces the contents of the given \p primFilter with \p dagPath, if
    /// a shape adapter for \p dagPath has already been batched. Returns true
    /// if successful. Otherwise, does not modify the \p primFilter, and
    /// returns false.
    ///
    /// The primFilter is used by Hydra to determine which prims should be
    /// present in the render path.  It consists of a collection
    /// (which prim paths to include/exclude) and Render Tags (which prim
    /// purposes to include).
    ///
    /// Note that the VP2 shape adapters are searched first, followed by the
    /// Legacy shape adapters. You cannot rely on the shape adapters being
    /// associated with a specific viewport.
    PXRUSDMAYAGL_API
    bool PopulateCustomPrimFilter(
            const MDagPath& dagPath,
            PxrMayaHdPrimFilter& primFilter);

    /// Render batch or bounding box in the legacy viewport based on \p request
    PXRUSDMAYAGL_API
    void Draw(const MDrawRequest& request, M3dView& view);

    /// Render batch or bounding box in Viewport 2.0 based on \p userData
    PXRUSDMAYAGL_API
    void Draw(
            const MHWRender::MDrawContext& context,
            const MUserData* userData);

    /// Gets the resolution of the draw target used for computing selections.
    ///
    /// The resolution is specified as (width, height).
    PXRUSDMAYAGL_API
    GfVec2i GetSelectionResolution() const;

    /// Sets the resolution of the draw target used for computing selections.
    ///
    /// The resolution should be specified as (width, height).
    ///
    /// Smaller values yield better performance but may miss selecting very
    /// small objects. Larger values will be slower but more accurate. The
    /// default resolution is (256, 256) for performance.
    PXRUSDMAYAGL_API
    void SetSelectionResolution(const GfVec2i& widthHeight);

    /// Gets whether depth selection has been enabled.
    PXRUSDMAYAGL_API
    bool IsDepthSelectionEnabled() const;

    /// Sets whether to enable depth selection.
    PXRUSDMAYAGL_API
    void SetDepthSelectionEnabled(const bool enabled);

    /// Tests the object from the given shape adapter for intersection with
    /// a given selection context in the legacy viewport.
    ///
    /// Returns a pointer to a hit set if there was an intersection, or nullptr
    /// otherwise.
    ///
    /// The returned HitSet is owned by the batch renderer, and it will be
    /// erased at the next selection, so clients should make copies if they
    /// need the data to persist.
    PXRUSDMAYAGL_API
    const HdxPickHitVector* TestIntersection(
            const PxrMayaHdShapeAdapter* shapeAdapter,
            MSelectInfo& selectInfo);

    /// Tests the object from the given shape adapter for intersection with
    /// a given draw context in Viewport 2.0.
    ///
    /// Returns a pointer to a hit set if there was an intersection, or nullptr
    /// otherwise.
    ///
    /// The returned HitSet is owned by the batch renderer, and it will be
    /// erased at the next selection, so clients should make copies if they
    /// need the data to persist.
    PXRUSDMAYAGL_API
    const HdxPickHitVector* TestIntersection(
            const PxrMayaHdShapeAdapter* shapeAdapter,
            const MHWRender::MSelectionInfo& selectionInfo,
            const MHWRender::MDrawContext& context);

    /// Tests the contents of the given prim filter (previously obtained
    /// via PopulateCustomFilter) for intersection with the current OpenGL
    /// context.
    /// The caller is responsible for ensuring that an appropriate OpenGL
    /// context is available; this function is not appropriate for interesecting
    /// using the Maya viewport.
    ///
    /// If hit(s) are found, returns \c true and populates \p *outResult with
    /// the intersection result.
    PXRUSDMAYAGL_API
    bool TestIntersectionCustomPrimFilter(
            const PxrMayaHdPrimFilter& primFilter,
            const GfMatrix4d& viewMatrix,
            const GfMatrix4d& projectionMatrix,
            HdxPickHitVector* outResult);

    /// Utility function for finding the nearest hit (in terms of ndcDepth) in
    /// the given \p hitSet.
    ///
    /// If \p hitSet is nullptr or is empty, nullptr is returned. Otherwise a
    /// pointer to the nearest hit in \p hitSet is returned.
    PXRUSDMAYAGL_API
    static const HdxPickHit* GetNearestHit(
            const HdxPickHitVector* hitSet);

    /// Returns the index within the point instancer for \c hit.
    ///
    /// Returns -1 if unable to get the instanceIndex.
    PXRUSDMAYAGL_API
    int GetInstancerIndexForHit(const HdxPickHit& hit) const;

    /// Returns whether soft selection for proxy shapes is currently enabled.
    PXRUSDMAYAGL_API
    inline bool GetObjectSoftSelectEnabled()
    { return _objectSoftSelectEnabled; }

    /// Starts batching all diagnostics until the end of the current frame draw.
    /// The batch renderer will automatically release the diagnostics when Maya
    /// is done rendering the frame.
    PXRUSDMAYAGL_API
    void StartBatchingFrameDiagnostics();

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

    typedef std::pair<PxrMayaHdRenderParams, PxrMayaHdPrimFilterVector>
            _RenderItem;

    /// Private helper function to render the given list of render items.
    /// Note that this doesn't set lighting, so if you need to update the
    /// lighting from the scene, you need to do that beforehand.
    void _Render(
            const GfMatrix4d& worldToViewMatrix,
            const GfMatrix4d& projectionMatrix,
            const GfVec4d& viewport,
            const std::vector<_RenderItem>& items);

    /// Call to render all queued batches. May be called safely without
    /// performance hit when no batches are queued.
    /// If \p view3d is null, then considers any isolated selection in that view
    /// when determining visibility. If it's null, then assumes that there's no
    /// isolated selection.
    void _RenderBatches(
            const MHWRender::MDrawContext* vp2Context,
            const M3dView* view3d,
            const GfMatrix4d& worldToViewMatrix,
            const GfMatrix4d& projectionMatrix,
            const GfVec4d& viewport);

    /// Private helper function for testing intersection on a single collection
    /// only.
    /// \returns True if there was at least one hit. All hits are returned in
    /// the \p *result.
    bool _TestIntersection(
            const HdRprimCollection& rprimCollection,
            const TfTokenVector& renderTags,
            const GfMatrix4d& viewMatrix,
            const GfMatrix4d& projectionMatrix,
            const bool singleSelection,
            HdxPickHitVector* result);

    // Handler for Maya scene resets (e.g. new scene or switch scenes).
    void _OnMayaSceneReset(const UsdMayaSceneResetNotice& notice);

    /// Handler for Maya Viewport 2.0 end render notifications.
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

    /// Record changes to soft select options
    ///
    /// This callback is just so we don't have to query the soft selection
    /// options through mel every time we have a selection event.
    static void _OnSoftSelectOptionsChangedCallback(void* clientData);

    /// Perform post-render state cleanup.
    ///
    /// For Viewport 2.0, this method gets invoked by
    /// _OnMayaEndRenderCallback() and is what does the actual cleanup work.
    /// For the legacy viewport, there is no such notification sent by Maya, so
    /// this method is called internally at the end of Hydra draws for the
    /// legacy viewport. In that case, vp2Context will be nullptr.
    void _MayaRenderDidEnd(const MHWRender::MDrawContext* vp2Context);

    /// Update the internal marker of whether a selection is pending.
    ///
    /// Returns true if the internal marker's value was changed, or false if
    /// the given value is the same as the current value.
    bool _UpdateIsSelectionPending(const bool isPending);

    /// Rendering invalidates selection, so when a render completes, we mark
    /// selection as pending.
    bool _isSelectionPending;

    bool _objectSoftSelectEnabled;
    MCallbackId _softSelectOptionsCallbackId;

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

    /// Mapping of Maya object handles to their shape adapters.
    /// This is a "secondary" container for storing shape adapters.
    typedef UsdMayaUtil::MObjectHandleUnorderedMap<PxrMayaHdShapeAdapter*>
            _ShapeAdapterHandleMap;

    /// We maintain separate object handle path maps for Viewport 2.0 and the
    /// legacy viewport.
    _ShapeAdapterHandleMap _shapeAdapterHandleMap;

    _ShapeAdapterHandleMap _legacyShapeAdapterHandleMap;

    /// We detect and store whether Viewport 2.0 is using the legacy
    /// viewport-based selection mechanism (i.e. whether the
    /// MAYA_VP2_USE_VP1_SELECTION environment variable is enabled) when the
    /// batch renderer is constructed. Then when a legacy selection is
    /// performed, we consult this value and the viewport renderer of the
    /// M3dView in which the selection is occurring to determine which bucket
    /// map of shape adapters we should use to compute the selection.
    bool _viewport2UsesLegacySelection;

    /// Gets the vector of prim filters to use for intersection testing.
    ///
    /// As an optimization for when we do not need to do intersection testing
    /// against all objects in depth (i.e. with single selections or when the
    /// PXRMAYAHD_ENABLE_DEPTH_SELECTION env setting is disabled), we use a
    /// single HdRprimCollection that includes all shape adapters/delegates
    /// registered with the batch renderer for the active viewport renderer
    /// (legacy viewport or Viewport 2.0), since we're only interested in the
    /// single nearest hit in depth for a particular pixel. This is much faster
    /// than testing against each shape adapter's prim filter individually.
    /// Otherwise, we test each shape adapter's prim filter individually so that
    /// occluded shapes will be included in the selection.
    PxrMayaHdPrimFilterVector _GetIntersectionPrimFilters(
            _ShapeAdapterBucketsMap& bucketsMap,
            const M3dView* view,
            const bool useDepthSelection) const;

    /// Populates the selection results using the given parameters by
    /// performing intersection tests against all of the shapes in the given
    /// \p bucketsMap.
    /// If \p view3d is null, then considers any isolated selection in that view
    /// when determining visibility. If it's null, then assumes that there's no
    /// isolated selection.
    void _ComputeSelection(
            _ShapeAdapterBucketsMap& bucketsMap,
            const M3dView* view3d,
            const GfMatrix4d& viewMatrix,
            const GfMatrix4d& projectionMatrix,
            const bool singleSelection);

    /// A cache of all selection results gathered since the last selection was
    /// computed. It maps delegate IDs to a HitSet of all of the intersection
    /// hits for that delegate ID.
    std::unordered_map<SdfPath, HdxPickHitVector, SdfPath::Hash> _selectResults;

    /// We keep track of a "key" that's associated with the select results.
    /// This is used to determine if the results can be shared among multiple
    /// shapes calling TestIntersection.
    typedef std::tuple<GfMatrix4d, GfMatrix4d, bool> _SelectResultsKey;
    _SelectResultsKey _selectResultsKey;

    /// Hydra engine objects used to render batches.
    ///
    /// Note that the Hydra render index is constructed with and is dependent
    /// on the render delegate. At destruction time, the render index uses the
    /// delegate to destroy Hydra prims, so the delegate must be destructed
    /// *after* the render index. We enforce that ordering by declaring the
    /// render delegate *before* the render index, since class members are
    /// destructed in reverse declaration order.
    HdEngine _hdEngine;
    HdStRenderDelegate _renderDelegate;
    std::unique_ptr<HdRenderIndex> _renderIndex;

    /// The root ID of the batch renderer itself, and the top of the path
    /// hierarchies for shape adapter delegates, one for the legacy viewport
    /// and one for Viewport 2.0.
    SdfPath _rootId;
    SdfPath _legacyViewportPrefix;
    SdfPath _viewport2Prefix;

    /// The batch renderer maintains a collection per viewport renderer that
    /// includes all shape adapters registered for that renderer.
    HdRprimCollection _legacyViewportRprimCollection;
    HdRprimCollection _viewport2RprimCollection;

    PxrMayaHdSceneDelegateSharedPtr _taskDelegate;

    GfVec2i _selectionResolution;
    bool _enableDepthSelection;

    HdxSelectionTrackerSharedPtr _selectionTracker;

    UsdMayaGLSoftSelectHelper _softSelectHelper;

    /// Shared diagnostic batch context. Used for cases where we want to batch
    /// diagnostics across multiple function calls, e.g., batching all of the
    /// Sync() diagnostics across all prepareForDraw() callbacks in a single
    /// frame.
    std::unique_ptr<UsdMayaDiagnosticBatchContext> _sharedDiagBatchCtx;
};

PXRUSDMAYAGL_API_TEMPLATE_CLASS(TfSingleton<UsdMayaGLBatchRenderer>);


PXR_NAMESPACE_CLOSE_SCOPE


#endif
