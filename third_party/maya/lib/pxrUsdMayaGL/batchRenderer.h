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
///
/// \file batchRenderer.h
///

#ifndef PXRUSDMAYAGL_BATCHRENDERER_H
#define PXRUSDMAYAGL_BATCHRENDERER_H

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRequest.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

#include <memory>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>

class MDagPath;

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
    static UsdMayaGLBatchRenderer& GetGlobalRenderer();

    struct RenderParams
    {
        // USD Params
        //
        UsdTimeCode frame = UsdTimeCode::Default();
        uint8_t refineLevel = 0;
        TfToken geometryCol = HdTokens->geometry;

        // Raster Params
        //
        bool enableLighting = true;
        
        // Geometry Params
        //
        HdCullStyle cullStyle = HdCullStyleNothing;
        TfToken drawRepr = HdTokens->refined;
        TfTokenVector renderTags;

        // Color Params
        //
        GfVec4f overrideColor = {.0f, .0f, .0f, .0f};
        GfVec4f wireframeColor = {.0f, .0f, .0f, .0f};
        
        /// \brief Helper function to find a batch key for the render params
        size_t Hash() const
        {
            size_t hash = (refineLevel<<1)+enableLighting;
            boost::hash_combine( hash, frame );
            boost::hash_combine( hash, geometryCol );
            boost::hash_combine( hash, cullStyle );
            boost::hash_combine( hash, drawRepr );
            boost::hash_combine( hash, overrideColor );
            boost::hash_combine( hash, wireframeColor );
            
            return hash;
        }
    };
    
    /// \brief Class to manage rendering of single Maya shape with a single
    /// non-instanced transform.
    class ShapeRenderer
    {
        friend class UsdMayaGLBatchRenderer;
        
    public:
            
        /// \brief Construct a new uninitialized \c ShapeRenderer.
        PXRUSDMAYAGL_API
        ShapeRenderer();

        PXRUSDMAYAGL_API
        void Init(HdRenderIndex *renderIndex,
                  const SdfPath& sharedId,
                  const UsdPrim& rootPrim,
                  const SdfPathVector& excludedPaths);
    
        /// \brief Register the \c ShapeRenderer with the specific DAG object
        /// in question. This should be called once per frame, per
        /// \c ShapeRenderer in use.
        PXRUSDMAYAGL_API
        void PrepareForQueue(
                const MDagPath& objPath,
                UsdTimeCode time,
                uint8_t refineLevel,
                bool showGuides,
                bool showRenderGuides,
                bool tint,
                GfVec4f tintColor );
    
        /// \brief Get a set of RenderParams from the VP 1.0 display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        RenderParams GetRenderParams(
                const MDagPath& objPath,
                const M3dView::DisplayStyle& displayStyle,
                const M3dView::DisplayStatus& displayStatus,
                bool* drawShape,
                bool* drawBoundingBox );

        /// \brief Get a set of RenderParams from the VP 1.0 display state.
        ///
        /// Sets \p drawShape and \p drawBoundingBox depending on whether shape
        /// and/or bounding box rendering is indicated from the state.
        PXRUSDMAYAGL_API
        RenderParams GetRenderParams(
                const MDagPath& objPath,
                const unsigned int& displayStyle,
                const MHWRender::DisplayStatus& displayStatus,
                bool* drawShape,
                bool* drawBoundingBox );

        /// \brief Queue a batch draw call, to be executed later in the
        /// specified \p mode.
        /// 
        /// \p boxToDraw may be set to NULL if no box is desired to be drawn.
        ///
        PXRUSDMAYAGL_API
        void QueueShapeForDraw(
                MPxSurfaceShapeUI *shapeUI,
                MDrawRequest& drawRequest,
                const RenderParams& params,
                bool drawShape,
                MBoundingBox *boxToDraw = NULL );

        /// \brief Queue a batch draw call, to be executed later in the
        /// specified \p mode.
        /// 
        /// \p userData should be the same parameter \c oldData passed into the
        /// caller: the overriden \c prepareForDraw(...) call. The \p userData
        /// pointer must also be returned from the overridden caller.
        /// 
        /// \p boxToDraw may be set to NULL if no box is desired to be drawn.
        ///
        PXRUSDMAYAGL_API
        void QueueShapeForDraw(
                MUserData* &userData,
                const RenderParams& params,
                bool drawShape,
                MBoundingBox *boxToDraw = NULL );

        /// \brief Tests an object for intersection with a given view.
        /// 
        /// \p hitPoint yields the point of interesection if \c true is returned.
        ///
        PXRUSDMAYAGL_API
        bool TestIntersection(
                M3dView& aView, 
                unsigned int pickResolution,
                bool singleSelection,
                GfVec3d* hitPoint) const; 

        /// \brief Returns base params as set previously by \c PrepareForQueue(...)
        ///
        const RenderParams& GetBaseParams() { return _baseParams; }
            
    private:
        SdfPath _sharedId;
        UsdPrim _rootPrim;
        SdfPathVector _excludedPaths;
        GfMatrix4d _rootXform;
        
        RenderParams _baseParams;
        
        bool _isPopulated;
        std::shared_ptr<UsdImagingDelegate> _delegate;
        UsdMayaGLBatchRenderer* _batchRenderer;
    };

    /// \brief hd task
    class TaskDelegate : public HdSceneDelegate {
    public:
        PXRUSDMAYAGL_API
        TaskDelegate(HdRenderIndex *renderIndex,
                     SdfPath const& delegateID);

        // HdSceneDelegate interface
        PXRUSDMAYAGL_API
        virtual VtValue Get(SdfPath const& id, TfToken const& key);

        PXRUSDMAYAGL_API
        void SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport);

        // VP 1.0 only.
        PXRUSDMAYAGL_API
        void SetLightingStateFromVP1(const MMatrix& viewMatForLights);

        // VP 2.0 only.
        PXRUSDMAYAGL_API
        void SetLightingStateFromMayaDrawContext(
                const MHWRender::MDrawContext& context);

        PXRUSDMAYAGL_API
        HdTaskSharedPtrVector GetSetupTasks();

        PXRUSDMAYAGL_API
        HdTaskSharedPtr GetRenderTask(size_t hash,
                                      RenderParams const &params,
                                      SdfPathVector const &roots);

    protected:
        PXRUSDMAYAGL_API
        void _InsertRenderTask(SdfPath const &id);

        PXRUSDMAYAGL_API
        void _SetLightingStateFromLightingContext();

        template <typename T>
        T const &_GetValue(SdfPath const &id, TfToken const &key) {
            VtValue vParams = _valueCacheMap[id][key];
            TF_VERIFY(vParams.IsHolding<T>());
            return vParams.Get<T>();
        }

        template <typename T>
        void _SetValue(SdfPath const &id, TfToken const &key, T const &value) {
            _valueCacheMap[id][key] = value;
        }

    private:
        typedef std::unordered_map<size_t, SdfPath> _RenderTaskIdMap;
        _RenderTaskIdMap _renderTaskIdMap;
        SdfPath _rootId;

        SdfPath _simpleLightTaskId;

        SdfPathVector _lightIds;
        SdfPath _cameraId;
        GfVec4d _viewport;

        GlfSimpleLightingContextRefPtr _lightingContext;

        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
    };
    typedef std::shared_ptr<TaskDelegate> TaskDelegateSharedPtr;
    
    /// \brief Gets a pointer to the \c ShapeRenderer associated with a certain
    /// set of parameters.
    ///
    /// The objected pointed to is owned by the
    /// \c UsdMayaGLBatchRenderer and will be valid for as long as the
    /// \c UsdMayaGLBatchRenderer object is valid.
    PXRUSDMAYAGL_API
    ShapeRenderer *GetShapeRenderer(
                    const UsdPrim& usdPrim, 
                    const SdfPathVector& excludePrimPaths,
                    const MDagPath& objPath );
    
    /// \brief Gets UsdMayaGLSoftSelectHelper that this batchRenderer maintains.
    /// This should only be used by ShapeRenderer::GetRenderParams
    PXRUSDMAYAGL_API
    const UsdMayaGLSoftSelectHelper& GetSoftSelectHelper();

    /// \brief Construct a new, unique BatchRenderer. In almost all cases,
    /// this should not be used -- use \c GetGlobalRenderer() instead.
    PXRUSDMAYAGL_API
    UsdMayaGLBatchRenderer();

    PXRUSDMAYAGL_API
    ~UsdMayaGLBatchRenderer();


    /// \brief Reset the internal state of the global UsdMayaGLBatchRenderer.
    /// In particular, it's important that this happen when switching to a new
    /// Maya scene so that any UsdImagingDelegates held by ShapeRenderers that
    /// have been populated with USD stages can have those stages released,
    /// since the delegates hold a strong pointers to their stages.
    PXRUSDMAYAGL_API
    static void Reset();

    /// \brief Render batch or bounds in VP1 based on \p request
    PXRUSDMAYAGL_API
    void Draw(
            const MDrawRequest& request,
            M3dView &view );
    
    /// \brief Render batch or bounds in VP2 based on \p userData
    PXRUSDMAYAGL_API
    void Draw(
            const MHWRender::MDrawContext& context,
            const MUserData *userData );
    
private:
    
    /// \brief Helper function to find a key for the shape in the renderer cache
    static size_t _ShapeHash(
            const UsdPrim& usdPrim, 
            const SdfPathVector& excludePrimPaths,
            const MDagPath& objPath );

    /// \brief Helper function for a \c ShapeRenderer to register a batch
    /// render call.
    void _QueueShapeForDraw(
            const SdfPath& sharedId,
            const RenderParams &params );
    
    /// \brief Helper function for a \c ShapeRenderer to register a batch
    /// render call.
    void _QueuePathForDraw(
            const SdfPath& sharedId,
            const RenderParams &params );
    
    /// \brief Tests an object for intersection with a given view.
    /// 
    /// \returns Hydra Hit info for instance associated with \p sharedId
    ///
    const HdxIntersector::Hit *_GetHitInfo(
            M3dView& aView, 
            unsigned int pickResolution,
            bool singleSelection,
            const SdfPath& sharedId,
            const GfMatrix4d &localToWorldSpace ); 

     /// \brief Call to render all queued batches. May be called safely w/o
    /// performance hit when no batches are queued.
    void _RenderBatches(
            const MHWRender::MDrawContext* vp2Context,
            const MMatrix& viewMat,
            const MMatrix& projectionMat,
            const GfVec4d& viewport );

    /// \brief Cache of hashed \c ShapeRenderer objects for fast lookup
    typedef std::unordered_map<size_t,ShapeRenderer> _ShapeRendererMap;
    _ShapeRendererMap _shapeRendererMap;
        
    /// \brief container of all delegates to be populated at next display
    /// refresh.
    std::unordered_set<ShapeRenderer *> _populateQueue;
        
    /// \brief Cache of \c SdfPath objects to be rendered
    typedef std::unordered_set<SdfPath, SdfPath::Hash> _SdfPathSet;
    
    /// \brief Associative pair of \c RenderParams and \c SdfPath objects to be
    /// rendered with said params
    typedef std::pair<RenderParams, _SdfPathSet> _RenderParamSet;
    
    /// \brief Lookup table to to find \c _RenderParamSet given a param hash key.
    typedef std::unordered_map<size_t,_RenderParamSet> _RendererQueueMap;

    /// \brief container of all batched render calls to be made at next display
    /// refresh.
    _RendererQueueMap _renderQueue;
        
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

    TaskDelegateSharedPtr _taskDelegate;
    HdxIntersectorSharedPtr _intersector;
    UsdMayaGLSoftSelectHelper _softSelectHelper;

    /// \brief Sole global batch renderer used by default.
    static std::unique_ptr<UsdMayaGLBatchRenderer> _sGlobalRendererPtr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_BATCHRENDERER_H
