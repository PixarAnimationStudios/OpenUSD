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
#include "pxrUsdMayaGL/softSelectHelper.h"

#include "px_vp20/utils.h"
#include "px_vp20/utils_legacy.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSceneMessage.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

#include <boost/functional/hash.hpp>
#include <boost/scoped_ptr.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (render)

    ((MayaEndRenderNotificationName, "UsdMayaEndRenderNotification"))
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(PXRUSDMAYAGL_QUEUE_INFO,
            "Prints out batch renderer queuing info.");
}

/* static */
void
UsdMayaGLBatchRenderer::Init()
{
    GlfGlewInit();

    Get();
}

/* static */
UsdMayaGLBatchRenderer&
UsdMayaGLBatchRenderer::Get()
{
    if (!_sGlobalRendererPtr) {
        Reset();
    }

    return *_sGlobalRendererPtr;
}

std::unique_ptr<UsdMayaGLBatchRenderer> UsdMayaGLBatchRenderer::_sGlobalRendererPtr;

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

        bool _drawShape;
        boost::scoped_ptr<MBoundingBox> _bounds;
        boost::scoped_ptr<GfVec4f> _wireframeColor;

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


UsdMayaGLBatchRenderer::ShapeRenderer::ShapeRenderer(
        const MDagPath& shapeDagPath,
        const UsdPrim& rootPrim,
        const SdfPathVector& excludedPrimPaths) :
    _shapeDagPath(shapeDagPath),
    _rootPrim(rootPrim),
    _excludedPrimPaths(excludedPrimPaths),
    _isPopulated(false)
{
}

size_t
UsdMayaGLBatchRenderer::ShapeRenderer::GetHash() const
{
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _rootPrim);
    boost::hash_combine(shapeHash, _excludedPrimPaths);

    return shapeHash;
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::Init(HdRenderIndex* renderIndex)
{
    const size_t shapeHash = GetHash();

    // Create a simple hash string to put into a flat SdfPath "hierarchy".
    // This is much faster than more complicated pathing schemes.
    const std::string idString = TfStringPrintf("/x%zx", shapeHash);
    _sharedId = SdfPath(idString);

    _delegate.reset(new UsdImagingDelegate(renderIndex, _sharedId));
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::PrepareForQueue(
        const UsdTimeCode time,
        const uint8_t refineLevel,
        const bool showGuides,
        const bool showRenderGuides,
        const bool tint,
        const GfVec4f& tintColor)
{
    // Initialization of default parameters go here. These parameters get used
    // in all viewports and for selection.
    //
    _baseParams.timeCode = time;
    _baseParams.refineLevel = refineLevel;

    // XXX Not yet adding ability to turn off display of proxy geometry, but
    // we should at some point, as in usdview
    _baseParams.renderTags.clear();
    _baseParams.renderTags.push_back(HdTokens->geometry);
    _baseParams.renderTags.push_back(HdTokens->proxy);
    if (showGuides) {
        _baseParams.renderTags.push_back(HdTokens->guide);
    }
    if (showRenderGuides) {
        _baseParams.renderTags.push_back(_tokens->render);
    }

    if (tint) {
        _baseParams.overrideColor = tintColor;
    }

    if (_delegate) {
        MStatus status;
        const MMatrix transform = _shapeDagPath.inclusiveMatrix(&status);
        if (status == MS::kSuccess) {
            _rootXform = GfMatrix4d(transform.matrix);
            _delegate->SetRootTransform(_rootXform);
        }

        _delegate->SetRefineLevelFallback(refineLevel);

        // Will only react if time actually changes.
        _delegate->SetTime(time);

        _delegate->SetRootCompensation(_rootPrim.GetPath());
    }
}


// Helper function that converts the M3dView::DisplayStatus (viewport 1.0)
// into the MHWRender::DisplayStatus (viewport 2.0)
static inline
MHWRender::DisplayStatus
_ToMHWRenderDisplayStatus(const M3dView::DisplayStatus& displayStatus)
{
    // these enums are equivalent, but statically checking just in case.
    static_assert(((int)M3dView::kActive == (int)MHWRender::kActive)
            && ((int)M3dView::kLive == (int)MHWRender::kLive)
            && ((int)M3dView::kDormant == (int)MHWRender::kDormant)
            && ((int)M3dView::kInvisible == (int)MHWRender::kInvisible)
            && ((int)M3dView::kHilite == (int)MHWRender::kHilite)
            && ((int)M3dView::kTemplate == (int)MHWRender::kTemplate)
            && ((int)M3dView::kActiveTemplate == (int)MHWRender::kActiveTemplate)
            && ((int)M3dView::kActiveComponent == (int)MHWRender::kActiveComponent)
            && ((int)M3dView::kLead == (int)MHWRender::kLead)
            && ((int)M3dView::kIntermediateObject == (int)MHWRender::kIntermediateObject)
            && ((int)M3dView::kActiveAffected == (int)MHWRender::kActiveAffected)
            && ((int)M3dView::kNoStatus == (int)MHWRender::kNoStatus),
            "M3dView::DisplayStatus == MHWRender::DisplayStatus");
    return MHWRender::DisplayStatus((int)displayStatus);
}

static
bool
_GetWireframeColor(
        const UsdMayaGLSoftSelectHelper& softSelectHelper,
        const MDagPath& objPath,
        const MHWRender::DisplayStatus& displayStatus,
        MColor* mayaWireColor)
{
    // dormant objects may be included in soft selection.
    if (displayStatus == MHWRender::kDormant) {
        return softSelectHelper.GetFalloffColor(objPath, mayaWireColor);
    }
    else if ((displayStatus == MHWRender::kActive) ||
             (displayStatus == MHWRender::kLead) ||
             (displayStatus == MHWRender::kHilite)) {
        *mayaWireColor = MHWRender::MGeometryUtilities::wireframeColor(objPath);
        return true;
    }

    return false;
}

PxrMayaHdRenderParams
UsdMayaGLBatchRenderer::ShapeRenderer::GetRenderParams(
        const MDagPath& objPath,
        const M3dView::DisplayStyle& displayStyle,
        const M3dView::DisplayStatus& displayStatus,
        bool* drawShape,
        bool* drawBoundingBox)
{
    // VP 1.0 Implementation
    //
    PxrMayaHdRenderParams params(_baseParams);

    // VP 1.0 deos not allow shapes and bounding boxes to be drawn at the
    // same time...
    //
    *drawBoundingBox = (displayStyle == M3dView::kBoundingBox);
    *drawShape = !*drawBoundingBox;

    // If obj is selected, we set selected and the wireframe color.
    // QueueShapeForDraw(...) will later break this single param set into
    // two, to perform a two-pass render.
    //
    MColor mayaWireframeColor;
    const bool needsWire = _GetWireframeColor(
        UsdMayaGLBatchRenderer::Get().GetSoftSelectHelper(),
        objPath,
        _ToMHWRenderDisplayStatus(displayStatus),
        &mayaWireframeColor);

    if (needsWire) {
        // The legacy viewport does not support color management,
        // so we roll our own gamma correction via framebuffer effect.
        // But that means we need to pre-linearize the wireframe color
        // from Maya.
        params.wireframeColor =
            GfConvertDisplayToLinear(GfVec4f(mayaWireframeColor.r,
                                             mayaWireframeColor.g,
                                             mayaWireframeColor.b,
                                             1.0f));
    }

    switch (displayStyle) {
        case M3dView::kWireFrame:
        {
            params.drawRepr = HdTokens->refinedWire;
            params.enableLighting = false;
            break;
        }
        case M3dView::kGouraudShaded:
        {
            if (needsWire) {
                params.drawRepr = HdTokens->refinedWireOnSurf;
            } else {
                params.drawRepr = HdTokens->refined;
            }
            break;
        }
        case M3dView::kFlatShaded:
        {
            if (needsWire) {
                params.drawRepr = HdTokens->wireOnSurf;
            } else {
                params.drawRepr = HdTokens->hull;
            }
            break;
        }
        case M3dView::kPoints:
        {
            // Points mode is not natively supported by Hydra, so skip it...
        }
        default:
        {
            *drawShape = false;
        }
    };

    return params;
}

PxrMayaHdRenderParams
UsdMayaGLBatchRenderer::ShapeRenderer::GetRenderParams(
        const MDagPath& objPath,
        const unsigned int& displayStyle,
        const MHWRender::DisplayStatus& displayStatus,
        bool* drawShape,
        bool* drawBoundingBox)
{
    // VP 2.0 Implementation
    //
    PxrMayaHdRenderParams params(_baseParams);

    *drawShape = true;
    *drawBoundingBox =
        (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    // If obj is selected, we set the wireframe color.
    // QueueShapeForDraw(...) will later break this single param set into
    // two, to perform a two-pass render.
    //
    MColor mayaWireframeColor;
    bool needsWire = _GetWireframeColor(
        UsdMayaGLBatchRenderer::Get().GetSoftSelectHelper(),
        objPath,
        displayStatus,
        &mayaWireframeColor);

    if (needsWire) {
        params.wireframeColor = GfVec4f(mayaWireframeColor.r,
                                        mayaWireframeColor.g,
                                        mayaWireframeColor.b,
                                        1.0f);
    }

// Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for whatever reason...
    const bool flatShaded =
#if MAYA_API_VERSION >= 201600
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
        false;
#endif

    if (flatShaded) {
        if (needsWire) {
            params.drawRepr = HdTokens->wireOnSurf;
        } else {
            params.drawRepr = HdTokens->hull;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded)
    {
        if (needsWire || (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)) {
            params.drawRepr = HdTokens->refinedWireOnSurf;
        } else {
            params.drawRepr = HdTokens->refined;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
    {
        params.drawRepr = HdTokens->refinedWire;
        params.enableLighting = false;
    }
    else
    {
        *drawShape = false;
    }

// Maya 2016 SP2 lacks MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling for whatever reason...
    params.cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        params.cullStyle = HdCullStyleBackUnlessDoubleSided;
    }
#endif

    return params;
}


UsdMayaGLBatchRenderer::ShapeRenderer*
UsdMayaGLBatchRenderer::GetShapeRenderer(
        const MDagPath& shapeDagPath,
        const UsdPrim& rootPrim,
        const SdfPathVector& excludedPrimPaths)
{
    ShapeRenderer shapeRenderer(shapeDagPath, rootPrim, excludedPrimPaths);
    const size_t shapeHash = shapeRenderer.GetHash();

    auto inserted = _shapeRendererMap.insert({shapeHash, shapeRenderer});
    ShapeRenderer* shapeRendererPtr = &inserted.first->second;

    if (inserted.second) {
        shapeRendererPtr->Init(_renderIndex.get());
    }

    return shapeRendererPtr;
}

void
UsdMayaGLBatchRenderer::QueueShapeForDraw(
        ShapeRenderer* shapeRenderer,
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

    QueueShapeForDraw(shapeRenderer,
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
        ShapeRenderer* shapeRenderer,
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
        _QueueShapeForDraw(shapeRenderer, params);
    }
}

void
UsdMayaGLBatchRenderer::_QueueShapeForDraw(
        ShapeRenderer* shapeRenderer,
        const PxrMayaHdRenderParams& params)
{
    if (!shapeRenderer->IsPopulated()) {
        // Populate this shape renderer on the next draw if it hasn't yet been
        // populated.
        _populateQueue.insert(shapeRenderer);
    }

    const size_t paramKey = params.Hash();

    auto renderSetIter = _renderQueue.find(paramKey);
    if (renderSetIter == _renderQueue.end()) {
        // If we had no _SdfPathSet for this particular RenderParam combination,
        // create a new one.
        _renderQueue[paramKey] =
            _RenderParamSet(params, _SdfPathSet({shapeRenderer->GetSharedId()}));
    } else {
        _SdfPathSet& renderPaths = renderSetIter->second.second;
        renderPaths.insert(shapeRenderer->GetSharedId());
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
    if (_sGlobalRendererPtr) {
        _sGlobalRendererPtr->_MayaRenderDidEnd();
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

    // the _shapeRendererMap has UsdImagingDelegate objects which need to be
    // deleted before _renderIndex is deleted.
    _shapeRendererMap.clear();
}

/* static */
void
UsdMayaGLBatchRenderer::Reset()
{
    if (_sGlobalRendererPtr) {
        MGlobal::displayInfo("Resetting USD Batch Renderer");
    }
    _sGlobalRendererPtr.reset(new UsdMayaGLBatchRenderer());
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
        const ShapeRenderer* shapeRenderer,
        M3dView& view,
        const unsigned int pickResolution,
        const bool singleSelection,
        GfVec3d* hitPoint)
{
    const HdxIntersector::Hit* hitInfo =
        _GetHitInfo(view,
                    pickResolution,
                    singleSelection,
                    shapeRenderer->GetSharedId(),
                    shapeRenderer->GetRootXform());
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
        const SdfPath& sharedId,
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

        for (const auto& renderSetIter : _selectQueue) {
            const PxrMayaHdRenderParams& renderParams = renderSetIter.second.first;
            const _SdfPathSet& renderPaths = renderSetIter.second.second;
            SdfPathVector roots(renderPaths.begin(), renderPaths.end());

            TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                "--- pickQueue, batch %zx, size %zu\n",
                renderSetIter.first,
                renderPaths.size());

            TfToken colName = renderParams.geometryCol;
            HdRprimCollection rprims(colName, renderParams.drawRepr);
            rprims.SetRootPaths(roots);
            rprims.SetRenderTags(renderParams.renderTags);

            qparams.cullStyle = renderParams.cullStyle;
            qparams.renderTags = renderParams.renderTags;

            HdxIntersector::Result result;
            HdxIntersector::HitVector hits;

            glPushAttrib(GL_VIEWPORT_BIT |
                         GL_ENABLE_BIT |
                         GL_COLOR_BUFFER_BIT |
                         GL_DEPTH_BUFFER_BIT |
                         GL_STENCIL_BUFFER_BIT |
                         GL_TEXTURE_BIT |
                         GL_POLYGON_BIT);
            bool r = _intersector->Query(qparams, rprims, &_hdEngine, &result);
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

    return TfMapLookupPtr(_selectResults, sharedId);
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

    if (!_populateQueue.empty()) {
        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "____________ POPULATE STAGE START ______________ (%zu)\n",
            _populateQueue.size());

        std::vector<UsdImagingDelegate*> delegates;
        UsdPrimVector rootPrims;
        std::vector<SdfPathVector> excludedPrimPaths;
        std::vector<SdfPathVector> invisedPrimPaths;

        for (ShapeRenderer* shapeRenderer : _populateQueue) {
            if (shapeRenderer->IsPopulated()) {
                continue;
            }

            delegates.push_back(shapeRenderer->GetDelegate());
            rootPrims.push_back(shapeRenderer->GetRootPrim());
            excludedPrimPaths.push_back(shapeRenderer->GetExcludedPrimPaths());
            invisedPrimPaths.push_back(SdfPathVector());

            shapeRenderer->SetPopulated(true);
        }

        UsdImagingDelegate::Populate(delegates,
                                     rootPrims,
                                     excludedPrimPaths,
                                     invisedPrimPaths);

        _populateQueue.clear();

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "^^^^^^^^^^^^ POPULATE STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n",
            _populateQueue.size());
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

    for (const auto& renderSetIter : _renderQueue) {
        const size_t hash = renderSetIter.first;
        const PxrMayaHdRenderParams& params = renderSetIter.second.first;
        const _SdfPathSet &renderPaths = renderSetIter.second.second;

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "*** renderQueue, batch %zx, size %zu\n",
            renderSetIter.first,
            renderPaths.size());

        SdfPathVector roots(renderPaths.begin(), renderPaths.end());
        tasks.push_back(_taskDelegate->GetRenderTask(hash, params, roots));
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
