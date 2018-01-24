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
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRequest.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MUserData.h>

#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEBUG_CODES(
    PXRUSDMAYAGL_QUEUE_INFO
);


typedef boost::shared_ptr<class HdxIntersector> HdxIntersectorSharedPtr;


/// \brief This is an helper object that shapes can hold to get consistent usd
/// batch drawing in maya, regardless of VP 1.0 or VP 2.0 usage.
///
/// Typical usage is as follows:
///
/// Every batched object can request a \c UsdMayaGLBatchRenderer::ShapeRenderer
/// object constructed from and cached within a shared batchRenderer.
///
/// At every refresh, in the prepare for draw stage, each ShapeRenderer should
/// first call \c PrepareForQueue(...) and then \c QueueShapeForDraw(...) for every
/// batch draw pass desired. \c GetRenderParams(...) should be used to construct
/// the render params used for consistency.
///
/// In the draw stage, \c Draw(...) must be called for each draw request to
/// complete the render.
///
class UsdMayaGLBatchRenderer : private boost::noncopyable
{
public:

    /// \brief Init the BatchRenderer class before using it.  This should be
    /// called at least once and it is OK to call it multiple times.  This
    /// handles things like initializing Gl/Glew.
    PXRUSDMAYAGL_API
    static void Init();

    PXRUSDMAYAGL_API
    static UsdMayaGLBatchRenderer& Get();

    /// \brief Class to manage rendering of single Maya shape with a single
    /// non-instanced transform.
    class ShapeRenderer
    {
        friend class UsdMayaGLBatchRenderer;

    public:

        /// \brief Construct a new uninitialized \c ShapeRenderer.
        PXRUSDMAYAGL_API
        ShapeRenderer(
                const MDagPath& shapeDagPath,
                const UsdPrim& rootPrim,
                const SdfPathVector& excludedPrimPaths);

        PXRUSDMAYAGL_API
        size_t GetHash() const;

        PXRUSDMAYAGL_API
        void Init(HdRenderIndex* renderIndex);

        /// \brief Prepare the shape renderer for being added to the batch
        /// renderer's queues. This should be called once per frame, per
        /// \c ShapeRenderer in use.
        PXRUSDMAYAGL_API
        void PrepareForQueue(
                const UsdTimeCode time,
                const uint8_t refineLevel,
                const bool showGuides,
                const bool showRenderGuides,
                const bool tint,
                const GfVec4f& tintColor);

        /// \brief Get a set of render params from the VP 1.0 display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        PxrMayaHdRenderParams GetRenderParams(
                const MDagPath& objPath,
                const M3dView::DisplayStyle& displayStyle,
                const M3dView::DisplayStatus& displayStatus,
                bool* drawShape,
                bool* drawBoundingBox);

        /// \brief Get a set of render params from the VP 2.0 display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        PxrMayaHdRenderParams GetRenderParams(
                const MDagPath& objPath,
                const unsigned int& displayStyle,
                const MHWRender::DisplayStatus& displayStatus,
                bool* drawShape,
                bool* drawBoundingBox);

        const UsdPrim& GetRootPrim() const { return _rootPrim; }

        const SdfPathVector& GetExcludedPrimPaths() const {
            return _excludedPrimPaths;
        }

        const GfMatrix4d& GetRootXform() const { return _rootXform; }

        /// \brief Returns base params as set previously by \c PrepareForQueue(...)
        ///
        const PxrMayaHdRenderParams& GetBaseParams() const { return _baseParams; }

        const SdfPath& GetSharedId() const { return _sharedId; }

        bool IsPopulated() const { return _isPopulated; }
        void SetPopulated(const bool isPopulated = true) {
            _isPopulated = isPopulated;
        }

        UsdImagingDelegate* GetDelegate() const {
            if (_delegate) {
                return _delegate.get();
            }

            return nullptr;
        }

    private:
        MDagPath _shapeDagPath;
        UsdPrim _rootPrim;
        SdfPathVector _excludedPrimPaths;
        GfMatrix4d _rootXform;

        PxrMayaHdRenderParams _baseParams;

        SdfPath _sharedId;
        bool _isPopulated;
        std::shared_ptr<UsdImagingDelegate> _delegate;
    };

    /// \brief Gets a pointer to the \c ShapeRenderer associated with a certain
    /// set of parameters.
    ///
    /// The object pointed to is owned by the \c UsdMayaGLBatchRenderer and
    /// will be valid for as long as the \c UsdMayaGLBatchRenderer object is
    /// valid.
    PXRUSDMAYAGL_API
    ShapeRenderer* GetShapeRenderer(
            const MDagPath& shapeDagPath,
            const UsdPrim& rootPrim,
            const SdfPathVector& excludedPrimPaths);

    /// \brief Queue a batch draw call, to be executed later.
    ///
    /// \p boxToDraw may be set to nullptr if no box is desired to be drawn.
    ///
    PXRUSDMAYAGL_API
    void QueueShapeForDraw(
            ShapeRenderer* shapeRenderer,
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
            ShapeRenderer* shapeRenderer,
            MUserData*& userData,
            const PxrMayaHdRenderParams& params,
            const bool drawShape,
            const MBoundingBox* boxToDraw = nullptr);

    /// \brief Gets UsdMayaGLSoftSelectHelper that this batchRenderer maintains.
    /// This should only be used by ShapeRenderer::GetRenderParams
    PXRUSDMAYAGL_API
    const UsdMayaGLSoftSelectHelper& GetSoftSelectHelper();

    /// \brief Construct a new, unique BatchRenderer. In almost all cases,
    /// this should not be used -- use \c Get() instead.
    PXRUSDMAYAGL_API
    UsdMayaGLBatchRenderer();

    PXRUSDMAYAGL_API
    virtual ~UsdMayaGLBatchRenderer();

    /// \brief Reset the internal state of the global UsdMayaGLBatchRenderer.
    /// In particular, it's important that this happen when switching to a new
    /// Maya scene so that any UsdImagingDelegates held by ShapeRenderers that
    /// have been populated with USD stages can have those stages released,
    /// since the delegates hold a strong pointers to their stages.
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
            const ShapeRenderer* shapeRenderer,
            M3dView& view,
            const unsigned int pickResolution,
            const bool singleSelection,
            GfVec3d* hitPoint);

private:

    /// \brief Private helper function for registering a batch render call.
    void _QueueShapeForDraw(
            ShapeRenderer* shapeRenderer,
            const PxrMayaHdRenderParams& params);

    /// \brief Tests an object for intersection with a given view.
    ///
    /// \returns Hydra Hit info for instance associated with \p sharedId
    ///
    const HdxIntersector::Hit* _GetHitInfo(
            M3dView& view,
            const unsigned int pickResolution,
            const bool singleSelection,
            const SdfPath& sharedId,
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

    /// \brief Cache of hashed \c ShapeRenderer objects for fast lookup
    typedef std::unordered_map<size_t, ShapeRenderer> _ShapeRendererMap;
    _ShapeRendererMap _shapeRendererMap;

    /// \brief container of all delegates to be populated at next display
    /// refresh.
    std::unordered_set<ShapeRenderer*> _populateQueue;

    /// \brief Cache of \c SdfPath objects to be rendered
    typedef std::unordered_set<SdfPath, SdfPath::Hash> _SdfPathSet;

    /// \brief Associative pair of \c PxrMayaHdRenderParams and \c SdfPath
    /// objects to be rendered with said params.
    typedef std::pair<PxrMayaHdRenderParams, _SdfPathSet> _RenderParamSet;

    /// \brief Lookup table to to find \c _RenderParamSet given a param hash key.
    typedef std::unordered_map<size_t, _RenderParamSet> _RendererQueueMap;

    /// \brief container of all batched render calls to be made at next display
    /// refresh.
    _RendererQueueMap _renderQueue;

    /// \brief Container of Maya render pass identifiers of passes drawn so far
    /// during a Viewport 2.0 render.
    ///
    /// Since all Hydra geometry is drawn at once, we only ever want to execute
    /// the Hydra draw once per Maya render pass (shadow, color, etc.). This
    /// container keeps track of which passes have been drawn by Hydra, and it
    /// is reset when the batch renderer is notified that a Maya render has
    /// ended.
    std::unordered_set<std::string> _drawnMayaRenderPasses;

    /// \brief container of batched render calls made at last display refresh,
    /// to be used at next selection operation.
    _RendererQueueMap _selectQueue;

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
    HdxIntersectorSharedPtr _intersector;
    UsdMayaGLSoftSelectHelper _softSelectHelper;

    /// \brief Sole global batch renderer used by default.
    static std::unique_ptr<UsdMayaGLBatchRenderer> _sGlobalRendererPtr;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_BATCH_RENDERER_H
