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

// glew needs to be included before any other OpenGL headers.
#include "pxr/imaging/glf/glew.h"

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/sceneDelegate.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "px_vp20/utils.h"
#include "px_vp20/utils_legacy.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSceneMessage.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

#include <memory>
#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((MayaEndRenderNotificationName, "UsdMayaEndRenderNotification"))
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(PXRUSDMAYAGL_QUEUE_INFO,
            "Prints out batch renderer queuing info.");
}


/// \brief struct to hold all the information needed for a
/// draw request in vp1 or vp2, without requiring shape querying at
/// draw time.
///
/// Note that we set deleteAfterUse=false when calling the MUserData
/// constructor. This ensures that the draw data survives across multiple draw
/// passes in vp2 (e.g. a shadow pass and a color pass).
class _BatchDrawUserData : public MUserData
{
    public:

        const bool _drawShape;
        const std::unique_ptr<MBoundingBox> _bounds;
        const std::unique_ptr<GfVec4f> _wireframeColor;

        // Constructor to use when shape is drawn but no bounding box.
        _BatchDrawUserData() :
            MUserData(/* deleteAfterUse = */ false),
            _drawShape(true) {}

        // Constructor to use when shape may be drawn but there is a bounding
        // box.
        _BatchDrawUserData(
                const bool drawShape,
                const MBoundingBox& bounds,
                const GfVec4f& wireFrameColor) :
            MUserData(/* deleteAfterUse = */ false),
            _drawShape(drawShape),
            _bounds(new MBoundingBox(bounds)),
            _wireframeColor(new GfVec4f(wireFrameColor)) {}

        // Make sure everything gets freed!
        virtual ~_BatchDrawUserData() {}
};


TF_INSTANTIATE_SINGLETON(UsdMayaGLBatchRenderer);

/* static */
void
UsdMayaGLBatchRenderer::Init()
{
    GlfGlewInit();

    GetInstance();
}

/* static */
UsdMayaGLBatchRenderer&
UsdMayaGLBatchRenderer::GetInstance()
{
    return TfSingleton<UsdMayaGLBatchRenderer>::GetInstance();
}

bool
UsdMayaGLBatchRenderer::AddShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    if (!shapeAdapter) {
        return false;
    }

    auto inserted = _shapeAdapterSet.insert(shapeAdapter);

    if (inserted.second) {
        shapeAdapter->Init(_renderIndex.get());
    }

    return inserted.second;
}

bool
UsdMayaGLBatchRenderer::RemoveShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    if (!shapeAdapter) {
        return false;
    }

    const size_t numErased = _shapeAdapterSet.erase(shapeAdapter);

    // Make sure we remove the shape adapter from the render and select queues
    // as well.
    for (auto& iter : _renderQueue) {
        _ShapeAdapterSet& shapeAdapters = iter.second.second;
        shapeAdapters.erase(shapeAdapter);
    }

    for (auto& iter : _selectQueue) {
        _ShapeAdapterSet& shapeAdapters = iter.second.second;
        shapeAdapters.erase(shapeAdapter);
    }

    return (numErased > 0u);
}

void
UsdMayaGLBatchRenderer::QueueShapeForDraw(
        PxrMayaHdShapeAdapter* shapeAdapter,
        MPxSurfaceShapeUI* shapeUI,
        MDrawRequest& drawRequest,
        const PxrMayaHdRenderParams& params,
        const bool drawShape,
        const MBoundingBox* boxToDraw)
{
    // VP 1.0 Implementation
    //
    // In Viewport 1.0, we can use MDrawData to communicate between the
    // draw prep and draw call itself. Since the internal data is open
    // to the client, we choose to use a MUserData object, so that the
    // internal data will mimic the the VP 2.0 implementation. This
    // allow for more code reuse.
    //
    // The one caveat here is that VP 1.0 does not manage the data allocated
    // in the MDrawData object, so we must remember to delete the MUserData
    // object in our Draw call.

    MUserData* userData;

    QueueShapeForDraw(shapeAdapter,
                      userData,
                      params,
                      drawShape,
                      boxToDraw);

    MDrawData drawData;
    shapeUI->getDrawData(userData, drawData);

    drawRequest.setDrawData(drawData);
}

void
UsdMayaGLBatchRenderer::QueueShapeForDraw(
        PxrMayaHdShapeAdapter* shapeAdapter,
        MUserData*& userData,
        const PxrMayaHdRenderParams& params,
        const bool drawShape,
        const MBoundingBox* boxToDraw)
{
    // VP 2.0 Implementation (also called by VP 1.0 Implementation)
    //
    // Our internal _BatchDrawUserData can be used to signify whether we are
    // requesting a shape to be rendered, a bounding box, both, or neither.
    //
    // If we aren't drawing the Shape, the userData object, passed by reference,
    // still gets set for the caller. In the VP 2.0 prepareForDraw(...) usage,
    // any MUserData object passed into the function will be deleted by Maya.
    // In the VP 1.0 usage, the object gets deleted in
    // UsdMayaGLBatchRenderer::Draw(...)

    if (boxToDraw) {
        userData = new _BatchDrawUserData(drawShape,
                                          *boxToDraw,
                                          params.wireframeColor);
    } else if (drawShape) {
        userData = new _BatchDrawUserData();
    } else {
        userData = nullptr;
    }

    if (drawShape) {
        _QueueShapeForDraw(shapeAdapter, params);
    }
}

void
UsdMayaGLBatchRenderer::_QueueShapeForDraw(
        PxrMayaHdShapeAdapter* shapeAdapter,
        const PxrMayaHdRenderParams& params)
{
    const size_t paramsHash = params.Hash();

    auto iter = _renderQueue.find(paramsHash);
    if (iter == _renderQueue.end()) {
        // If we had no shape adapter set for this particular RenderParam
        // combination, create a new one.
        _renderQueue[paramsHash] =
            _RenderParamSet(params, _ShapeAdapterSet({shapeAdapter}));
    } else {
        _ShapeAdapterSet& shapeAdapters = iter->second.second;
        shapeAdapters.insert(shapeAdapter);
    }
}

const UsdMayaGLSoftSelectHelper&
UsdMayaGLBatchRenderer::GetSoftSelectHelper()
{
    _softSelectHelper.Populate();
    return _softSelectHelper;
}

// Since we're using a static singleton UsdMayaGLBatchRenderer object, we need
// to make sure that we reset its state when switching to a new Maya scene.
static
void
_OnMayaSceneUpdateCallback(void* clientData)
{
    UsdMayaGLBatchRenderer::Reset();
}

// For Viewport 2.0, we listen for a notification from Maya's rendering
// pipeline that all render passes have completed and then we do some cleanup.
/* static */
void
UsdMayaGLBatchRenderer::_OnMayaEndRenderCallback(
        MHWRender::MDrawContext& context,
        void* clientData)
{
    if (UsdMayaGLBatchRenderer::CurrentlyExists()) {
        UsdMayaGLBatchRenderer::GetInstance()._MayaRenderDidEnd();
    }
}

UsdMayaGLBatchRenderer::UsdMayaGLBatchRenderer()
{
    _renderIndex.reset(HdRenderIndex::New(&_renderDelegate));
    if (!TF_VERIFY(_renderIndex)) {
        return;
    }
    _taskDelegate.reset(
        new PxrMayaHdSceneDelegate(_renderIndex.get(), SdfPath("/mayaTask")));
    _intersector.reset(new HdxIntersector(_renderIndex.get()));

    static MCallbackId sceneUpdateCallbackId = 0;
    if (sceneUpdateCallbackId == 0) {
        sceneUpdateCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kSceneUpdate,
                                       _OnMayaSceneUpdateCallback);
    }

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        MGlobal::displayError("Viewport 2.0 renderer not initialized.");
    } else {
        // Note that we do not ever remove this notification handler. Maya
        // ensures that only one handler will be registered for a given name
        // and semantic location.
        renderer->addNotification(
            UsdMayaGLBatchRenderer::_OnMayaEndRenderCallback,
            _tokens->MayaEndRenderNotificationName.GetText(),
            MHWRender::MPassContext::kEndRenderSemantic,
            nullptr);
    }
}

/* virtual */
UsdMayaGLBatchRenderer::~UsdMayaGLBatchRenderer()
{
    _intersector.reset();
    _taskDelegate.reset();
}

/* static */
void
UsdMayaGLBatchRenderer::Reset()
{
    if (UsdMayaGLBatchRenderer::CurrentlyExists()) {
        MGlobal::displayInfo("Resetting USD Batch Renderer");
        UsdMayaGLBatchRenderer::DeleteInstance();
    }

    UsdMayaGLBatchRenderer::GetInstance();
}

void
UsdMayaGLBatchRenderer::Draw(const MDrawRequest& request, M3dView& view)
{
    // VP 1.0 Implementation
    //
    MDrawData drawData = request.drawData();

    _BatchDrawUserData* batchData =
        static_cast<_BatchDrawUserData*>(drawData.geometry());
    if (!batchData) {
        return;
    }

    MMatrix projectionMat;
    view.projectionMatrix(projectionMat);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    if (batchData->_bounds) {
        MMatrix modelViewMat;
        view.modelViewMatrix(modelViewMat);

        px_vp20Utils::RenderBoundingBox(*(batchData->_bounds),
                                        *(batchData->_wireframeColor),
                                        modelViewMat,
                                        projectionMat);
    }

    if (batchData->_drawShape && !_renderQueue.empty()) {
        // Note that we use GfMatrix4d's GetInverse() method to get the
        // world-to-view matrix from the camera matrix and NOT MMatrix's
        // inverse(). The latter was introducing very small bits of floating
        // point error that would sometimes result in the positions of lights
        // being computed downstream as having w coordinate values that were
        // very close to but not exactly 1.0 or 0.0. When drawn, the light
        // would then flip between being a directional light (w = 0.0) and a
        // non-directional light (w = 1.0).
        MDagPath cameraDagPath;
        view.getCamera(cameraDagPath);
        const GfMatrix4d cameraMatrix(cameraDagPath.inclusiveMatrix().matrix);
        const GfMatrix4d worldToViewMatrix(cameraMatrix.GetInverse());

        unsigned int viewX, viewY, viewWidth, viewHeight;
        view.viewport(viewX, viewY, viewWidth, viewHeight);
        const GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

        // Only the first call to this will do anything... After that the batch
        // queue is cleared.
        //
        _RenderBatches(nullptr, worldToViewMatrix, projectionMatrix, viewport);
    }

    // Clean up the _BatchDrawUserData!
    delete batchData;
}

void
UsdMayaGLBatchRenderer::Draw(
        const MHWRender::MDrawContext& context,
        const MUserData* userData)
{
    // VP 2.0 Implementation
    //
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer || !theRenderer->drawAPIIsOpenGL()) {
        return;
    }

    const _BatchDrawUserData* batchData =
        static_cast<const _BatchDrawUserData*>(userData);
    if (!batchData) {
        return;
    }

    MStatus status;

    const MMatrix projectionMat =
        context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    if (batchData->_bounds) {
        const MMatrix worldViewMat =
            context.getMatrix(MHWRender::MFrameContext::kWorldViewMtx, &status);

        px_vp20Utils::RenderBoundingBox(*(batchData->_bounds),
                                        *(batchData->_wireframeColor),
                                        worldViewMat,
                                        projectionMat);
    }

    const MHWRender::MPassContext& passContext = context.getPassContext();
    const MString& passId = passContext.passIdentifier();

    const auto inserted = _drawnMayaRenderPasses.insert(passId.asChar());
    if (!inserted.second) {
        // We've already done a Hydra draw for this Maya render pass, so we
        // don't do another one.
        return;
    }

    if (batchData->_drawShape && !_renderQueue.empty()) {
        const MMatrix viewMat =
            context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
        const GfMatrix4d worldToViewMatrix(viewMat.matrix);

        // Extract camera settings from maya view
        int viewX, viewY, viewWidth, viewHeight;
        context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);
        const GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

        // Only the first call to this will do anything... After that the batch
        // queue is cleared.
        //
        _RenderBatches(&context, worldToViewMatrix, projectionMatrix, viewport);
    }
}

bool
UsdMayaGLBatchRenderer::TestIntersection(
        const PxrMayaHdShapeAdapter* shapeAdapter,
        M3dView& view,
        const unsigned int pickResolution,
        const bool singleSelection,
        GfVec3d* hitPoint)
{
    const HdxIntersector::Hit* hitInfo =
        _GetHitInfo(view,
                    pickResolution,
                    singleSelection,
                    shapeAdapter->GetDelegateID(),
                    shapeAdapter->GetRootXform());
    if (!hitInfo) {
        return false;
    }

    if (hitPoint) {
        *hitPoint = hitInfo->worldSpaceHitPoint;
    }

    if (TfDebug::IsEnabled(PXRUSDMAYAGL_QUEUE_INFO)) {
        cout << "FOUND HIT: " << endl;
        cout << "    delegateId: " << hitInfo->delegateId << endl;
        cout << "    objectId  : " << hitInfo->objectId << endl;
        cout << "    ndcDepth  : " << hitInfo->ndcDepth << endl;
    }

    return true;
}

const HdxIntersector::Hit*
UsdMayaGLBatchRenderer::_GetHitInfo(
        M3dView& view,
        const unsigned int pickResolution,
        const bool singleSelection,
        const SdfPath& delegateId,
        const GfMatrix4d& localToWorldSpace)
{
    // Guard against user clicking in viewer before renderer is setup
    if (!_renderIndex) {
        return nullptr;
    }

    // Selection only occurs once per display refresh, with all usd objects
    // simulataneously. If the selectQueue is not empty, that means that
    // a refresh has occurred, and we need to perform a new selection operation.

    if (!_selectQueue.empty()) {
        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "____________ SELECTION STAGE START ______________ (singleSelect = %d)\n",
            singleSelection);

        GfMatrix4d viewMatrix;
        GfMatrix4d projectionMatrix;
        px_LegacyViewportUtils::GetViewSelectionMatrices(view,
                                                         &viewMatrix,
                                                         &projectionMatrix);

        // As Maya doesn't support batched selection, intersection testing is
        // actually performed in the first selection query that happens after a
        // render. This query occurs in the local space of SOME object, but
        // we need results in world space so that we have results for every
        // node available. worldToLocalSpace removes the local space we
        // happen to be in for the initial query.
        GfMatrix4d worldToLocalSpace(localToWorldSpace.GetInverse());

        _intersector->SetResolution(GfVec2i(pickResolution, pickResolution));

        HdxIntersector::Params qparams;
        qparams.viewMatrix = worldToLocalSpace * viewMatrix;
        qparams.projectionMatrix = projectionMatrix;
        qparams.alphaThreshold = 0.1f;

        _selectResults.clear();

        for (const auto& iter : _selectQueue) {
            const size_t paramsHash = iter.first;
            const PxrMayaHdRenderParams& renderParams = iter.second.first;
            const _ShapeAdapterSet& shapeAdapters = iter.second.second;

            HdRprimCollectionVector rprimCollections;
            for (const PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
                rprimCollections.push_back(shapeAdapter->GetRprimCollection());
            }

            TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                "--- pickQueue, batch %zx, size %zu\n",
                paramsHash,
                rprimCollections.size());

            qparams.cullStyle = renderParams.cullStyle;

            for (const HdRprimCollection& rprimCollection : rprimCollections) {
                qparams.renderTags = rprimCollection.GetRenderTags();

                HdxIntersector::Result result;
                HdxIntersector::HitVector hits;

                glPushAttrib(GL_VIEWPORT_BIT |
                             GL_ENABLE_BIT |
                             GL_COLOR_BUFFER_BIT |
                             GL_DEPTH_BUFFER_BIT |
                             GL_STENCIL_BUFFER_BIT |
                             GL_TEXTURE_BIT |
                             GL_POLYGON_BIT);
                const bool r = _intersector->Query(qparams,
                                                   rprimCollection,
                                                   &_hdEngine,
                                                   &result);
                glPopAttrib();
                if (!r) {
                    continue;
                }

                if (singleSelection) {
                    hits.resize(1);
                    if (!result.ResolveNearest(&hits.front())) {
                        continue;
                    }
                }
                else if (!result.ResolveAll(&hits)) {
                    continue;
                }

                for (const HdxIntersector::Hit& hit : hits) {
                    auto itIfExists =
                        _selectResults.insert(
                            std::pair<SdfPath, HdxIntersector::Hit>(hit.delegateId, hit));

                    const bool &inserted = itIfExists.second;
                    if (inserted) {
                        continue;
                    }

                    HdxIntersector::Hit& existingHit = itIfExists.first->second;
                    if (hit.ndcDepth < existingHit.ndcDepth) {
                        existingHit = hit;
                    }
                }
            }
        }

        if (singleSelection && _selectResults.size() > 1u) {
            TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                "!!! multiple singleSel hits found: %zu\n",
                _selectResults.size());

            auto minIt = _selectResults.begin();
            for (auto curIt = minIt; curIt != _selectResults.end(); ++curIt) {
                const HdxIntersector::Hit& curHit = curIt->second;
                const HdxIntersector::Hit& minHit = minIt->second;
                if (curHit.ndcDepth < minHit.ndcDepth) {
                    minIt = curIt;
                }
            }

            if (minIt != _selectResults.begin()) {
                _selectResults.erase(_selectResults.begin(), minIt);
            }
            ++minIt;
            if (minIt != _selectResults.end()) {
                _selectResults.erase(minIt, _selectResults.end());
            }
        }

        if (TfDebug::IsEnabled(PXRUSDMAYAGL_QUEUE_INFO)) {
            for (const auto& selectPair : _selectResults) {
                const SdfPath& path = selectPair.first;
                const HdxIntersector::Hit& hit = selectPair.second;
                cout << "NEW HIT       : " << path << endl;
                cout << "    delegateId: " << hit.delegateId << endl;
                cout << "    objectId  : " << hit.objectId << endl;
                cout << "    ndcDepth  : " << hit.ndcDepth << endl;
            }
        }

        // As we've cached the results in pickBatches, we
        // can clear out the selection queue.
        _selectQueue.clear();

        // Selection can happen after a refresh but before a draw call, so
        // clear out the render queue as well
        _renderQueue.clear();

        // If nothing was selected, the view does not refresh, but this
        // means _selectQueue will not get processed again even if the
        // user attempts another selection. We fix the renderer state by
        // scheduling another refresh when the view is next idle.

        if (_selectResults.empty()) {
            view.scheduleRefresh();
        }

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "^^^^^^^^^^^^ SELECTION STAGE FINISH ^^^^^^^^^^^^^\n");
    }

    return TfMapLookupPtr(_selectResults, delegateId);
}

void
UsdMayaGLBatchRenderer::_RenderBatches(
        const MHWRender::MDrawContext* vp2Context,
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix,
        const GfVec4d& viewport)
{
    if (_renderQueue.empty()) {
        return;
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "____________ RENDER STAGE START ______________ (%zu)\n",
        _renderQueue.size());

    // A new display refresh signifies that the cached selection data is no
    // longer valid.
    _selectQueue.clear();
    _selectResults.clear();

    // We've already populated with all the selection info we need.  We Reset
    // and the first call to GetSoftSelectHelper in the next render pass will
    // re-populate it.
    _softSelectHelper.Reset();

    _taskDelegate->SetCameraState(worldToViewMatrix,
                                  projectionMatrix,
                                  viewport);

    // save the current GL states which hydra may reset to default
    glPushAttrib(GL_LIGHTING_BIT |
                 GL_ENABLE_BIT |
                 GL_POLYGON_BIT |
                 GL_DEPTH_BUFFER_BIT |
                 GL_VIEWPORT_BIT);

    // hydra orients all geometry during topological processing so that
    // front faces have ccw winding. We disable culling because culling
    // is handled by fragment shader discard.
    glFrontFace(GL_CCW); // < State is pushed via GL_POLYGON_BIT
    glDisable(GL_CULL_FACE);

    // note: to get benefit of alpha-to-coverage, the target framebuffer
    // has to be a MSAA buffer.
    glDisable(GL_BLEND);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    if (vp2Context) {
        _taskDelegate->SetLightingStateFromMayaDrawContext(*vp2Context);
    } else {
        _taskDelegate->SetLightingStateFromVP1(worldToViewMatrix,
                                               projectionMatrix);
    }

    // The legacy viewport does not support color management,
    // so we roll our own gamma correction by GL means (only in
    // non-highlight mode)
    bool gammaCorrect = !vp2Context;

    if (gammaCorrect) {
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render task setup
    HdTaskSharedPtrVector tasks = _taskDelegate->GetSetupTasks(); // lighting etc

    for (const auto& iter : _renderQueue) {
        const size_t paramsHash = iter.first;
        const PxrMayaHdRenderParams& params = iter.second.first;
        const _ShapeAdapterSet& shapeAdapters = iter.second.second;

        HdRprimCollectionVector rprimCollections;
        for (const PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
            rprimCollections.push_back(shapeAdapter->GetRprimCollection());
        }

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "*** renderQueue, batch %zx, size %zu\n",
            paramsHash,
            rprimCollections.size());

        tasks.push_back(
            _taskDelegate->GetRenderTask(paramsHash, params, rprimCollections));
    }

    _hdEngine.Execute(*_renderIndex, tasks);

    if (gammaCorrect) {
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    }

    glPopAttrib(); // GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT

    // Viewport 2 may be rendering in multiple passes, and we want to make sure
    // we draw once (and only once) for each of those passes, so we delay
    // swapping the render queue into the select queue until we receive a
    // notification that all rendering has ended.
    // For the legacy viewport, rendering is done in a single pass and we will
    // not receive a notification at the end of rendering, so we do the swap
    // now.
    if (!vp2Context) {
        _MayaRenderDidEnd();
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n",
        _renderQueue.size());
}

void
UsdMayaGLBatchRenderer::_MayaRenderDidEnd()
{
    // Selection is based on what we have last rendered to the display. The
    // selection queue is cleared during drawing, so this has the effect of
    // resetting the render queue and prepping the selection queue without any
    // significant memory hit.
    _renderQueue.swap(_selectQueue);

    _drawnMayaRenderPasses.clear();
}


PXR_NAMESPACE_CLOSE_SCOPE
