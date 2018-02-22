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
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"
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
#include <maya/MSelectionContext.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypes.h>
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
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_QUEUE_INFO,
        "Prints out batch renderer queuing info.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING,
        "Reports on changes in the sets of shape adapters registered with the "
        "batch renderer.");
}


/// Struct to hold all the information needed for a draw request in the legacy
/// viewport or Viewport 2.0, without requiring shape querying at draw time.
///
/// Note that we set deleteAfterUse=false when calling the MUserData
/// constructor. This ensures that the draw data survives across multiple draw
/// passes in Viewport 2.0 (e.g. a shadow pass and a color pass).
class _BatchDrawUserData : public MUserData
{
    public:

        const bool drawShape;
        const std::unique_ptr<MBoundingBox> bounds;
        const std::unique_ptr<GfVec4f> wireframeColor;

        // Constructor to use when shape is drawn but no bounding box.
        _BatchDrawUserData() :
            MUserData(/* deleteAfterUse = */ false),
            drawShape(true) {}

        // Constructor to use when shape may be drawn but there is a bounding
        // box.
        _BatchDrawUserData(
                const bool drawShape,
                const MBoundingBox& bounds,
                const GfVec4f& wireFrameColor) :
            MUserData(/* deleteAfterUse = */ false),
            drawShape(drawShape),
            bounds(new MBoundingBox(bounds)),
            wireframeColor(new GfVec4f(wireFrameColor)) {}

        // Make sure everything gets freed!
        virtual ~_BatchDrawUserData() override {}
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

HdRenderIndex*
UsdMayaGLBatchRenderer::GetRenderIndex() const
{
    return _renderIndex.get();
}

bool
UsdMayaGLBatchRenderer::AddShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    if (!TF_VERIFY(shapeAdapter, "Cannot add invalid shape adapter")) {
        return false;
    }

    const bool isViewport2 = shapeAdapter->IsViewport2();

    _ShapeAdapterBucketsMap& bucketsMap = isViewport2 ?
        _shapeAdapterBuckets :
        _legacyShapeAdapterBuckets;

    const PxrMayaHdRenderParams renderParams =
        shapeAdapter->GetRenderParams(nullptr, nullptr);
    const size_t renderParamsHash = renderParams.Hash();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
        "Adding shape adapter: %p, isViewport2: %s, renderParamsHash: %zu\n",
        shapeAdapter,
        isViewport2 ? "true" : "false",
        renderParamsHash);

    auto iter = bucketsMap.find(renderParamsHash);
    if (iter == bucketsMap.end()) {
        // If we had no bucket for this particular render param combination
        // then we need to create a new one, but first we make sure that the
        // shape adapter isn't in any other bucket.
        RemoveShapeAdapter(shapeAdapter);

        bucketsMap[renderParamsHash] =
            _ShapeAdapterBucket(renderParams, _ShapeAdapterSet({shapeAdapter}));

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
            "    Added to newly created bucket\n");
    } else {
        // Check whether this shape adapter is already in this bucket.
        _ShapeAdapterSet& shapeAdapters = iter->second.second;
        auto setIter = shapeAdapters.find(shapeAdapter);
        if (setIter != shapeAdapters.end()) {
            // If it's already in this bucket, then we're done, and we didn't
            // have to add it.
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                "    Not adding, already in correct bucket\n");

            return false;
        }

        // Otherwise, remove it from any other bucket it may be in before
        // adding it to this one.
        RemoveShapeAdapter(shapeAdapter);

        shapeAdapters.insert(shapeAdapter);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
            "    Added to existing bucket\n");
    }

    // Debug dumping of current bucket state.
    if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
            "    _shapeAdapterBuckets (Viewport 2.0) contents:\n");
        for (const auto& iter : _shapeAdapterBuckets) {
            const size_t bucketHash = iter.first;
            const _ShapeAdapterSet& shapeAdapters = iter.second.second;

            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                "        renderParamsHash: %zu, bucket size: %zu\n",
                bucketHash,
                shapeAdapters.size());

            for (const auto shapeAdapter : shapeAdapters) {
                TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                    "            shape adapter: %p\n",
                    shapeAdapter);
            }
        }

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
            "    _legacyShapeAdapterBuckets (Legacy viewport) contents:\n");
        for (auto& iter : _legacyShapeAdapterBuckets) {
            const size_t bucketHash = iter.first;
            const _ShapeAdapterSet& shapeAdapters = iter.second.second;

            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                "        renderParamsHash: %zu, bucket size: %zu\n",
                bucketHash,
                shapeAdapters.size());

            for (const auto shapeAdapter : shapeAdapters) {
                TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                    "            shape adapter: %p\n",
                    shapeAdapter);
            }
        }
    }

    return true;
}

bool
UsdMayaGLBatchRenderer::RemoveShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    if (!TF_VERIFY(shapeAdapter, "Cannot remove invalid shape adapter")) {
        return false;
    }

    const bool isViewport2 = shapeAdapter->IsViewport2();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
        "Removing shape adapter: %p, isViewport2: %s\n",
        shapeAdapter,
        isViewport2 ? "true" : "false");

    _ShapeAdapterBucketsMap& bucketsMap = isViewport2 ?
        _shapeAdapterBuckets :
        _legacyShapeAdapterBuckets;

    size_t numErased = 0u;

    std::vector<size_t> emptyBucketHashes;

    for (auto& iter : bucketsMap) {
        const size_t renderParamsHash = iter.first;
        _ShapeAdapterSet& shapeAdapters = iter.second.second;

        const size_t numBefore = numErased;
        numErased += shapeAdapters.erase(shapeAdapter);

        if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING) &&
                numErased > numBefore) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                "    Removed from bucket with render params hash: %zu\n",
                renderParamsHash);
        }

        if (shapeAdapters.empty()) {
            // This bucket is now empty, so we tag it for removal below.
            emptyBucketHashes.push_back(renderParamsHash);
        }
    }

    // Remove any empty buckets.
    for (const size_t renderParamsHash : emptyBucketHashes) {
        const size_t numErasedBuckets = bucketsMap.erase(renderParamsHash);

        if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING) &&
                numErasedBuckets > 0u) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg(
                "    Removed empty bucket with render params hash: %zu\n",
                renderParamsHash);
        }
    }

    return (numErased > 0u);
}

void
UsdMayaGLBatchRenderer::CreateBatchDrawData(
        MPxSurfaceShapeUI* shapeUI,
        MDrawRequest& drawRequest,
        const PxrMayaHdRenderParams& params,
        const bool drawShape,
        const MBoundingBox* boxToDraw)
{
    // Legacy viewport implementation.

    // If we're in this method, we must be prepping for a legacy viewport
    // render, so mark a legacy render as pending.
    _UpdateLegacyRenderPending(true);

    MUserData* userData;

    CreateBatchDrawData(userData,
                        params,
                        drawShape,
                        boxToDraw);

    // Note that the legacy viewport does not manage the data allocated in the
    // MDrawData object, so we must remember to delete the MUserData object at
    // the end of our Draw() call.
    MDrawData drawData;
    shapeUI->getDrawData(userData, drawData);

    drawRequest.setDrawData(drawData);
}

void
UsdMayaGLBatchRenderer::CreateBatchDrawData(
        MUserData*& userData,
        const PxrMayaHdRenderParams& params,
        const bool drawShape,
        const MBoundingBox* boxToDraw)
{
    // Viewport 2.0 implementation (also called by legacy viewport
    // implementation).
    //
    // Our internal _BatchDrawUserData can be used to signify whether we are
    // requesting a shape to be rendered, a bounding box, both, or neither.
    //
    // If we aren't drawing the Shape, the userData object, passed by reference,
    // still gets set for the caller. In the Viewport 2.0 prepareForDraw()
    // usage, any MUserData object passed into the function will be deleted by
    // Maya. In the legacy viewport usage, the object gets deleted in
    // UsdMayaGLBatchRenderer::Draw().

    if (boxToDraw) {
        userData = new _BatchDrawUserData(drawShape,
                                          *boxToDraw,
                                          params.wireframeColor);
    } else if (drawShape) {
        userData = new _BatchDrawUserData();
    } else {
        userData = nullptr;
    }
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
        UsdMayaGLBatchRenderer::GetInstance()._MayaRenderDidEnd(&context);
    }
}

UsdMayaGLBatchRenderer::UsdMayaGLBatchRenderer() :
        _lastRenderFrameStamp(0u),
        _lastSelectionFrameStamp(0u),
        _legacyRenderPending(false),
        _legacySelectionPending(false)
{
    _viewport2UsesLegacySelection = TfGetenvBool("MAYA_VP2_USE_VP1_SELECTION",
                                                 false);

    _renderIndex.reset(HdRenderIndex::New(&_renderDelegate));
    if (!TF_VERIFY(_renderIndex)) {
        return;
    }

    _taskDelegate.reset(
        new PxrMayaHdSceneDelegate(_renderIndex.get(),
                                   SdfPath("/MayaHdSceneDelegate")));

    _intersector.reset(new HdxIntersector(_renderIndex.get()));
    _selectionTracker.reset(new HdxSelectionTracker());

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
    _selectionTracker.reset();
    _intersector.reset();
    _taskDelegate.reset();
}

const UsdMayaGLSoftSelectHelper&
UsdMayaGLBatchRenderer::GetSoftSelectHelper()
{
    _softSelectHelper.Populate();
    return _softSelectHelper;
}

static
void
_GetWorldToViewMatrix(M3dView& view, GfMatrix4d* worldToViewMatrix)
{
    // Legacy viewport implementation.

    if (!worldToViewMatrix) {
        return;
    }

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
    *worldToViewMatrix = cameraMatrix.GetInverse();
}

static
void
_GetViewport(M3dView& view, GfVec4d* viewport)
{
    // Legacy viewport implementation.

    if (!viewport) {
        return;
    }

    unsigned int viewX, viewY, viewWidth, viewHeight;
    view.viewport(viewX, viewY, viewWidth, viewHeight);
    *viewport = GfVec4d(viewX, viewY, viewWidth, viewHeight);
}

static
void
_GetWorldToViewMatrix(
        const MHWRender::MDrawContext& context,
        GfMatrix4d* worldToViewMatrix)
{
    // Viewport 2.0 implementation.

    if (!worldToViewMatrix) {
        return;
    }

    MStatus status;
    const MMatrix viewMat =
        context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    *worldToViewMatrix = GfMatrix4d(viewMat.matrix);
}

static
void
_GetViewport(const MHWRender::MDrawContext& context, GfVec4d* viewport)
{
    // Viewport 2.0 implementation.

    if (!viewport) {
        return;
    }

    int viewX, viewY, viewWidth, viewHeight;
    context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);
    *viewport = GfVec4d(viewX, viewY, viewWidth, viewHeight);
}

void
UsdMayaGLBatchRenderer::Draw(const MDrawRequest& request, M3dView& view)
{
    // Legacy viewport implementation.

    MDrawData drawData = request.drawData();

    const _BatchDrawUserData* batchData =
        static_cast<const _BatchDrawUserData*>(drawData.geometry());
    if (!batchData) {
        return;
    }

    MMatrix projectionMat;
    view.projectionMatrix(projectionMat);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    if (batchData->bounds) {
        MMatrix modelViewMat;
        view.modelViewMatrix(modelViewMat);

        px_vp20Utils::RenderBoundingBox(*(batchData->bounds),
                                        *(batchData->wireframeColor),
                                        modelViewMat,
                                        projectionMat);
    }

    if (batchData->drawShape && _UpdateLegacyRenderPending(false)) {
        GfMatrix4d worldToViewMatrix;
        _GetWorldToViewMatrix(view, &worldToViewMatrix);

        GfVec4d viewport;
        _GetViewport(view, &viewport);

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
    // Viewport 2.0 implementation.

    const MHWRender::MRenderer* theRenderer =
        MHWRender::MRenderer::theRenderer();
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

    if (batchData->bounds) {
        const MMatrix worldViewMat =
            context.getMatrix(MHWRender::MFrameContext::kWorldViewMtx, &status);

        px_vp20Utils::RenderBoundingBox(*(batchData->bounds),
                                        *(batchData->wireframeColor),
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

    const MUint64 frameStamp = context.getFrameStamp();

    if (batchData->drawShape && _UpdateRenderFrameStamp(frameStamp)) {
        GfMatrix4d worldToViewMatrix;
        _GetWorldToViewMatrix(context, &worldToViewMatrix);

        GfVec4d viewport;
        _GetViewport(context, &viewport);

        _RenderBatches(&context, worldToViewMatrix, projectionMatrix, viewport);
    }
}

bool
UsdMayaGLBatchRenderer::TestIntersection(
        const PxrMayaHdShapeAdapter* shapeAdapter,
        M3dView& view,
        const bool singleSelection,
        GfVec3f* hitPoint)
{
    // Legacy viewport implementation.
    //
    // HOWEVER... we may actually be performing a selection for Viewport 2.0 if
    // the MAYA_VP2_USE_VP1_SELECTION environment variable is set. If the
    // view's renderer is Viewport 2.0 AND it is using the legacy
    // viewport-based selection method, we compute the selection against the
    // Viewport 2.0 shape adapter buckets rather than the legacy buckets, since
    // we want to compute selection against what's actually being rendered.

    bool useViewport2Buckets = false;

    MStatus status;
    const M3dView::RendererName rendererName = view.getRendererName(&status);
    if (status == MS::kSuccess &&
            rendererName == M3dView::kViewport2Renderer &&
            _viewport2UsesLegacySelection) {
        useViewport2Buckets = true;
    }

    _ShapeAdapterBucketsMap& bucketsMap = useViewport2Buckets ?
        _shapeAdapterBuckets :
        _legacyShapeAdapterBuckets;

    // Guard against the user clicking in the viewer before the renderer is
    // setup, or with no shape adapters registered.
    if (!_renderIndex || bucketsMap.empty()) {
        _selectResults.clear();
        return false;
    }

    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;
    px_LegacyViewportUtils::GetViewSelectionMatrices(view,
                                                     &viewMatrix,
                                                     &projectionMatrix);

    // In the legacy viewport, selection occurs in the local space of SOME
    // object, but we need the view matrix in world space to correctly consider
    // all nodes. Applying localToWorldSpace removes the local space we happen
    // to be in.
    const GfMatrix4d localToWorldSpace(shapeAdapter->GetRootXform().GetInverse());
    viewMatrix = localToWorldSpace * viewMatrix;

    if (_UpdateLegacySelectionPending(false)) {
        _ComputeSelection(bucketsMap,
                          viewMatrix,
                          projectionMatrix,
                          singleSelection);
    }

    const HdxIntersector::Hit* hitInfo =
        TfMapLookupPtr(_selectResults, shapeAdapter->GetDelegateID());
    if (!hitInfo) {
        if (_selectResults.empty()) {
            // If nothing was selected previously AND nothing is selected now,
            // Maya does not refresh the viewport. This would be fine, except
            // that we need to make sure we're ready to respond to another
            // selection. Maya may be calling select() on many shapes in
            // series, so we cannot mark a legacy selection pending here or we
            // will end up re-computing the selection on every call. Instead we
            // simply schedule a refresh of the viewport, at the end of which
            // the end render callback will be invoked and we'll mark a legacy
            // selection pending then.
            // This is not an issue with Viewport 2.0, since in that case we
            // have the draw context's frame stamp to uniquely identify the
            // selection operation.
            view.scheduleRefresh();
        }

        return false;
    }

    if (hitPoint) {
        *hitPoint = hitInfo->worldSpaceHitPoint;
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "FOUND HIT:\n"
        "    delegateId: %s\n"
        "    objectId  : %s\n"
        "    ndcDepth  : %f\n",
        hitInfo->delegateId.GetText(),
        hitInfo->objectId.GetText(),
        hitInfo->ndcDepth);

    return true;
}

bool
UsdMayaGLBatchRenderer::TestIntersection(
        const PxrMayaHdShapeAdapter* shapeAdapter,
        const MHWRender::MSelectionInfo& selectInfo,
        const MHWRender::MDrawContext& context,
        const bool singleSelection,
        GfVec3f* hitPoint)
{
    // Viewport 2.0 implementation.

    // Guard against the user clicking in the viewer before the renderer is
    // setup, or with no shape adapters registered.
    if (!_renderIndex || _shapeAdapterBuckets.empty()) {
        _selectResults.clear();
        return false;
    }

    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;
    if (!px_vp20Utils::GetSelectionMatrices(selectInfo,
                                            context,
                                            viewMatrix,
                                            projectionMatrix)) {
        return false;
    }

    if (_UpdateSelectionFrameStamp(context.getFrameStamp())) {
        _ComputeSelection(_shapeAdapterBuckets,
                          viewMatrix,
                          projectionMatrix,
                          singleSelection);
    }

    const HdxIntersector::Hit* hitInfo =
        TfMapLookupPtr(_selectResults, shapeAdapter->GetDelegateID());
    if (!hitInfo) {
        return false;
    }

    if (hitPoint) {
        *hitPoint = hitInfo->worldSpaceHitPoint;
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "FOUND HIT:\n"
        "    delegateId: %s\n"
        "    objectId  : %s\n"
        "    ndcDepth  : %f\n",
        hitInfo->delegateId.GetText(),
        hitInfo->objectId.GetText(),
        hitInfo->ndcDepth);

    return true;
}

void
UsdMayaGLBatchRenderer::_ComputeSelection(
        _ShapeAdapterBucketsMap& bucketsMap,
        const GfMatrix4d& viewMatrix,
        const GfMatrix4d& projectionMatrix,
        const bool singleSelection)
{
    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "____________ SELECTION STAGE START ______________ (singleSelect = %d)\n",
        singleSelection);

    // We may miss very small objects with this setting, but it's faster.
    const unsigned int pickResolution = 256u;

    _intersector->SetResolution(GfVec2i(pickResolution, pickResolution));

    HdxIntersector::Params qparams;
    qparams.viewMatrix = viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = 0.1f;

    _selectResults.clear();

    for (const auto& iter : bucketsMap) {
        const size_t paramsHash = iter.first;
        const _ShapeAdapterSet& shapeAdapters = iter.second.second;

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "--- pickBucket, parameters hash: %zu, bucket size %zu\n",
            paramsHash,
            shapeAdapters.size());

        for (PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
            shapeAdapter->UpdateVisibility();

            const HdRprimCollection& rprimCollection =
                shapeAdapter->GetRprimCollection();

            // Re-query the shape adapter for the render params rather than
            // using what's in the queue.
            const PxrMayaHdRenderParams& renderParams =
                shapeAdapter->GetRenderParams(nullptr, nullptr);

            qparams.renderTags = rprimCollection.GetRenderTags();
            qparams.cullStyle = renderParams.cullStyle;

            HdxIntersector::Result result;

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

            HdxIntersector::HitSet hits;

            if (singleSelection) {
                HdxIntersector::Hit hit;
                if (!result.ResolveNearest(&hit)) {
                    continue;
                }

                hits.insert(hit);
            }
            else if (!result.ResolveUnique(&hits)) {
                continue;
            }

            for (const HdxIntersector::Hit& hit : hits) {
                auto itIfExists =
                    _selectResults.insert(
                        std::make_pair(hit.delegateId, hit));

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

    // Populate the Hydra selection from the selection results.
    HdxSelectionSharedPtr selection(new HdxSelection);

    const HdxSelectionHighlightMode selectionMode =
        HdxSelectionHighlightModeSelect;

    for (const auto& selectPair : _selectResults) {
        const SdfPath& path = selectPair.first;
        const HdxIntersector::Hit& hit = selectPair.second;

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "NEW HIT          : %s\n"
            "    delegateId   : %s\n"
            "    objectId     : %s\n"
            "    instanceIndex: %d\n"
            "    ndcDepth     : %f\n",
            path.GetText(),
            hit.delegateId.GetText(),
            hit.objectId.GetText(),
            hit.instanceIndex,
            hit.ndcDepth);

        if (!hit.instancerId.IsEmpty()) {
            VtIntArray instanceIndices;
            instanceIndices.push_back(hit.instanceIndex);
            selection->AddInstance(selectionMode, hit.objectId, instanceIndices);
        } else {
            selection->AddRprim(selectionMode, hit.objectId);
        }
    }

    _selectionTracker->SetSelection(selection);

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "^^^^^^^^^^^^ SELECTION STAGE FINISH ^^^^^^^^^^^^^\n");
}

void
UsdMayaGLBatchRenderer::_RenderBatches(
        const MHWRender::MDrawContext* vp2Context,
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix,
        const GfVec4d& viewport)
{
    _ShapeAdapterBucketsMap& bucketsMap = bool(vp2Context) ?
        _shapeAdapterBuckets :
        _legacyShapeAdapterBuckets;

    if (bucketsMap.empty()) {
        return;
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "____________ RENDER STAGE START ______________ (%zu buckets)\n",
        bucketsMap.size());

    // A new display refresh signifies that the cached selection data is no
    // longer valid.
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
    const bool gammaCorrect = !vp2Context;

    if (gammaCorrect) {
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render task setup
    HdTaskSharedPtrVector tasks = _taskDelegate->GetSetupTasks(); // lighting etc

    for (const auto& iter : bucketsMap) {
        const size_t paramsHash = iter.first;
        const PxrMayaHdRenderParams& params = iter.second.first;
        const _ShapeAdapterSet& shapeAdapters = iter.second.second;

        HdRprimCollectionVector rprimCollections;
        for (PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
            shapeAdapter->UpdateVisibility();

            rprimCollections.push_back(shapeAdapter->GetRprimCollection());
        }

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "*** renderBucket, parameters hash: %zu, bucket size %zu\n",
            paramsHash,
            rprimCollections.size());

        HdTaskSharedPtrVector renderTasks =
            _taskDelegate->GetRenderTasks(paramsHash, params, rprimCollections);
        tasks.insert(tasks.end(), renderTasks.begin(), renderTasks.end());
    }

    VtValue selectionTrackerValue(_selectionTracker);
    _hdEngine.SetTaskContextData(HdxTokens->selectionState,
                                 selectionTrackerValue);

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
        _MayaRenderDidEnd(nullptr);
    }

    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^\n");
}

bool
UsdMayaGLBatchRenderer::_UpdateRenderFrameStamp(const MUint64 frameStamp)
{
    if (_lastRenderFrameStamp == frameStamp) {
        return false;
    }

    _lastRenderFrameStamp = frameStamp;

    return true;
}

bool
UsdMayaGLBatchRenderer::_UpdateSelectionFrameStamp(const MUint64 frameStamp)
{
    if (_lastSelectionFrameStamp == frameStamp) {
        return false;
    }

    _lastSelectionFrameStamp = frameStamp;

    return true;
}

bool
UsdMayaGLBatchRenderer::_UpdateLegacyRenderPending(const bool isPending)
{
    if (_legacyRenderPending == isPending) {
        return false;
    }

    _legacyRenderPending = isPending;

    return true;
}

bool
UsdMayaGLBatchRenderer::_UpdateLegacySelectionPending(const bool isPending)
{
    if (_legacySelectionPending == isPending) {
        return false;
    }

    _legacySelectionPending = isPending;

    return true;
}

void
UsdMayaGLBatchRenderer::_MayaRenderDidEnd(
        const MHWRender::MDrawContext* /* context */)
{
    // Note that we mark a legacy selection as pending regardless of which
    // viewport renderer is active. This is to ensure that selection works
    // correctly in case the MAYA_VP2_USE_VP1_SELECTION environment variable is
    // being used, in which case even though Viewport 2.0 (MPxDrawOverrides)
    // will be doing the drawing, the legacy viewport (MPxSurfaceShapeUIs) will
    // be handling selection.
    _UpdateLegacySelectionPending(true);

    _drawnMayaRenderPasses.clear();
}


PXR_NAMESPACE_CLOSE_SCOPE
