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
// Some header #define's Bool as int, which breaks stuff in sdf/types.h.
// Include it first to sidestep the problem. :-/
#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"

#include "pxr/imaging/hdSt/camera.h"
#include "pxr/imaging/hdSt/light.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/registryManager.h"

#include "px_vp20/utils.h"
#include "px_vp20/utils_legacy.h"
#include "pxrUsdMayaGL/batchRenderer.h"

#include <maya/M3dView.h>
#include <maya/MDrawData.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSceneMessage.h>

#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (idRenderTask)
    (renderTask)
    (selectionTask)
    (simpleLightTask)
    (camera)
    (render)
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

    GetGlobalRenderer();
}

/* static */
UsdMayaGLBatchRenderer&
UsdMayaGLBatchRenderer::GetGlobalRenderer()
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
class _BatchDrawUserData : public MUserData
{
public:

    bool _drawShape;
    boost::scoped_ptr<MBoundingBox> _bounds;
    boost::scoped_ptr<GfVec4f> _wireframeColor;

    // Constructor to use when shape is drawn but no bounding box.
    _BatchDrawUserData()
        : MUserData(true), _drawShape(true) {}
    
    // Constructor to use when shape may be drawn but there is a bounding box.
    _BatchDrawUserData(bool drawShape, const MBoundingBox &bounds, const GfVec4f &wireFrameColor)
        : MUserData( true )
        , _drawShape( drawShape )
        , _bounds( new MBoundingBox(bounds) )
        ,  _wireframeColor( new GfVec4f(wireFrameColor) ) {}

    // Make sure everything gets freed!
    ~_BatchDrawUserData() {}
};


UsdMayaGLBatchRenderer::ShapeRenderer::ShapeRenderer()
    : _isPopulated(false)
    , _batchRenderer( NULL )
{
    // _batchRenderer is NULL to signify an uninitialized ShapeRenderer.
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::Init(
    HdRenderIndex *renderIndex,
    const SdfPath& sharedId,
    const UsdPrim& rootPrim,
    const SdfPathVector& excludedPaths)
{
    _sharedId = sharedId;
    _rootPrim = rootPrim;
    _excludedPaths = excludedPaths;

    _delegate.reset(new UsdImagingDelegate(renderIndex, _sharedId));
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::PrepareForQueue(
    const MDagPath &objPath,
    UsdTimeCode time,
    uint8_t refineLevel,
    bool showGuides,
    bool showRenderGuides,
    bool tint,
    GfVec4f tintColor )
{
    // Initialization of default parameters go here. These parameters get used
    // in all viewports and for selection.
    //
    _baseParams.frame = time;
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
    
    if( tint )
        _baseParams.overrideColor = tintColor;

    if( _delegate )
    {
        MStatus status;
        MMatrix transform = objPath.inclusiveMatrix( &status );
        if( status )
        {
            _rootXform = GfMatrix4d( transform.matrix );
            _delegate->SetRootTransform(_rootXform);
        }

        _delegate->SetRefineLevelFallback(refineLevel);

        // Will only react if time actually changes.
        _delegate->SetTime(time);

        _delegate->SetRootCompensation(_rootPrim.GetPath());

        if( !_isPopulated )
            _batchRenderer->_populateQueue.insert(this);
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
            (displayStatus == MHWRender::kLead)    || 
            (displayStatus == MHWRender::kHilite)) {
        *mayaWireColor = MHWRender::MGeometryUtilities::wireframeColor(objPath);
        return true;
    }

    return false;
}

UsdMayaGLBatchRenderer::RenderParams
UsdMayaGLBatchRenderer::ShapeRenderer::GetRenderParams(
    const MDagPath& objPath,
    const M3dView::DisplayStyle& displayStyle,
    const M3dView::DisplayStatus& displayStatus,
    bool* drawShape,
    bool* drawBoundingBox )
{
    // VP 1.0 Implementation
    //
    RenderParams params( _baseParams );
    
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
    bool needsWire = _GetWireframeColor(
            _batchRenderer->GetSoftSelectHelper(),
            objPath, 
            _ToMHWRenderDisplayStatus(displayStatus), 
            &mayaWireframeColor);

    if( needsWire )
    {
        // The legacy viewport does not support color management,
        // so we roll our own gamma correction via framebuffer effect.
        // But that means we need to pre-linearize the wireframe color
        // from Maya.
        params.wireframeColor =
                GfConvertDisplayToLinear(
                    GfVec4f(
                        mayaWireframeColor.r,
                        mayaWireframeColor.g,
                        mayaWireframeColor.b,
                        1.f ));
    }
    
    switch( displayStyle )
    {
        case M3dView::kWireFrame:
        {
            params.drawRepr = HdTokens->refinedWire;
            params.enableLighting = false;
            break;
        }
        case M3dView::kGouraudShaded:
        {
            if( needsWire )
                params.drawRepr = HdTokens->refinedWireOnSurf;
            else
                params.drawRepr = HdTokens->refined;
            break;
        }
        case M3dView::kFlatShaded:
        {
            if( needsWire )
                params.drawRepr = HdTokens->wireOnSurf;
            else
                params.drawRepr = HdTokens->hull;
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

UsdMayaGLBatchRenderer::RenderParams
UsdMayaGLBatchRenderer::ShapeRenderer::GetRenderParams(
    const MDagPath& objPath,
    const unsigned int& displayStyle,
    const MHWRender::DisplayStatus& displayStatus,
    bool* drawShape,
    bool* drawBoundingBox )
{
    // VP 2.0 Implementation
    //
    RenderParams params( _baseParams );
    
    *drawShape = true;
    *drawBoundingBox = (displayStyle
                            & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    // If obj is selected, we set the wireframe color.
    // QueueShapeForDraw(...) will later break this single param set into
    // two, to perform a two-pass render.
    //
    MColor mayaWireframeColor;
    bool needsWire = _GetWireframeColor(
            _batchRenderer->GetSoftSelectHelper(),
            objPath, displayStatus,
            &mayaWireframeColor);
    
    if( needsWire )
    {
        params.wireframeColor =
                    GfVec4f(
                        mayaWireframeColor.r,
                        mayaWireframeColor.g,
                        mayaWireframeColor.b,
                        1.f );
    }
    
// Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for whatever reason...
    bool flatShaded =
#if MAYA_API_VERSION >= 201600
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
        false;
#endif
    
    if( flatShaded )
    {
        if( needsWire )
            params.drawRepr = HdTokens->wireOnSurf;
        else
            params.drawRepr = HdTokens->hull;
    }
    else if( displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded )
    {
        if( needsWire || (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame) )
            params.drawRepr = HdTokens->refinedWireOnSurf;
        else
            params.drawRepr = HdTokens->refined;
    }
    else if( displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame )
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
    if( displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling )
        params.cullStyle = HdCullStyleBackUnlessDoubleSided;
#endif
    
    return params;
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::QueueShapeForDraw(
    MPxSurfaceShapeUI *shapeUI,
    MDrawRequest &drawRequest,
    const RenderParams &params,
    bool drawShape,
    MBoundingBox *boxToDraw )
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
    
    QueueShapeForDraw( userData, params, drawShape, boxToDraw );
    
    MDrawData drawData;
    shapeUI->getDrawData( userData, drawData );
    
    drawRequest.setDrawData( drawData );
}

void
UsdMayaGLBatchRenderer::ShapeRenderer::QueueShapeForDraw(
    MUserData* &userData,
    const RenderParams &params,
    bool drawShape,
    MBoundingBox *boxToDraw )
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
    
    if( boxToDraw )
        userData = new _BatchDrawUserData( drawShape, *boxToDraw, params.wireframeColor );
    else if( drawShape )
        userData = new _BatchDrawUserData();
    else
        userData = NULL;
    
    if( drawShape && _batchRenderer )
        _batchRenderer->_QueueShapeForDraw( _sharedId, params );
}

bool 
UsdMayaGLBatchRenderer::ShapeRenderer::TestIntersection(
    M3dView& view,
    unsigned int pickResolution,
    bool singleSelection,
    GfVec3d* hitPoint) const
{
    const HdxIntersector::Hit *hitInfo =
            _batchRenderer->_GetHitInfo(
                view, pickResolution, singleSelection, _sharedId, _rootXform );
    if( !hitInfo )
        return false;
    
    if( hitPoint )
        *hitPoint = hitInfo->worldSpaceHitPoint;
    
    if( TfDebug::IsEnabled(PXRUSDMAYAGL_QUEUE_INFO) )
    {
        cout << "FOUND HIT: " << endl;
        cout << "\tdelegateId: " << hitInfo->delegateId << endl;
        cout << "\tobjectId: " << hitInfo->objectId << endl;
        cout << "\tndcDepth: " << hitInfo->ndcDepth << endl;
    }
    
    return true;
}

UsdMayaGLBatchRenderer::TaskDelegate::TaskDelegate(
    HdRenderIndex *renderIndex, SdfPath const& delegateID)
    : HdSceneDelegate(renderIndex, delegateID)
{
    _lightingContext = GlfSimpleLightingContext::New();

    // populate tasks in renderindex

    // create an unique namespace
    _rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_UsdImaging_%p", this)));

    _simpleLightTaskId          = _rootId.AppendChild(_tokens->simpleLightTask);
    _cameraId                   = _rootId.AppendChild(_tokens->camera);

    // camera
    {
        // Since we're hardcoded to use HdStRenderDelegate, we expect to
        // have camera Sprims.
        TF_VERIFY(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera));

        renderIndex->InsertSprim(HdPrimTypeTokens->camera, this, _cameraId);
        _ValueCache &cache = _valueCacheMap[_cameraId];
        cache[HdStCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->windowPolicy] = VtValue();  // no window policy.
    }

    // simple lighting task (for Hydra native)
    {
        renderIndex->InsertTask<HdxSimpleLightTask>(this, _simpleLightTaskId);
        _ValueCache &cache = _valueCacheMap[_simpleLightTaskId];
        HdxSimpleLightTaskParams taskParams;
        taskParams.cameraPath = _cameraId;
        cache[HdTokens->params] = VtValue(taskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }
}

void
UsdMayaGLBatchRenderer::TaskDelegate::_InsertRenderTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxRenderTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxRenderTaskParams taskParams;
    taskParams.camera = _cameraId;
    // Initialize viewport to the latest value since render tasks can be lazily
    // instantiated, potentially even after SetCameraState.  All other
    // parameters will be updated by _UpdateRenderParams.
    taskParams.viewport = _viewport;
    cache[HdTokens->params] = VtValue(taskParams);
    cache[HdTokens->children] = VtValue(SdfPathVector());
    cache[HdTokens->collection] = VtValue();
}

/*virtual*/
VtValue
UsdMayaGLBatchRenderer::TaskDelegate::Get(
    SdfPath const& id,
    TfToken const &key)
{
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if( vcache && TfMapLookup(*vcache, key, &ret) )
        return ret;

    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
                    id.GetText(), key.GetText());
    return VtValue();
}

void
UsdMayaGLBatchRenderer::TaskDelegate::SetCameraState(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache &cache = _valueCacheMap[_cameraId];
    cache[HdStCameraTokens->worldToViewMatrix] = VtValue(viewMatrix);
    cache[HdStCameraTokens->projectionMatrix] = VtValue(projectionMatrix);
    cache[HdStCameraTokens->windowPolicy] = VtValue(); // no window policy.

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkSprimDirty(_cameraId,
                                                       HdStCamera::AllDirty);

    if( _viewport != viewport )
    {
        // viewport is also read by HdxRenderTaskParam. invalidate it.
        _viewport = viewport;

        // update all render tasks
        for( const auto &it : _renderTaskIdMap )
        {
            SdfPath const &taskId = it.second;
            HdxRenderTaskParams taskParams
                = _GetValue<HdxRenderTaskParams>(taskId, HdTokens->params);

            // update viewport in HdxRenderTaskParams
            taskParams.viewport = viewport;
            _SetValue(taskId, HdTokens->params, taskParams);

            // invalidate
            GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                taskId, HdChangeTracker::DirtyParams);
        }
    }
}

void
UsdMayaGLBatchRenderer::TaskDelegate::SetLightingStateFromVP1(
            const MMatrix& viewMatForLights)
{
    // This function should only be called in a VP1.0 context. In VP2.0, we can
    // translate the lighting state from the MDrawContext directly into Glf,
    // but there is no draw context in VP1.0, so we have to transfer the state
    // through OpenGL.
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(viewMatForLights.matrix[0]);
    _lightingContext->SetStateFromOpenGL();
    glPopMatrix();

    _SetLightingStateFromLightingContext();
}

void
UsdMayaGLBatchRenderer::TaskDelegate::SetLightingStateFromMayaDrawContext(
        const MHWRender::MDrawContext& context)
{
    _lightingContext = px_vp20Utils::GetLightingContextFromDrawContext(context);

    _SetLightingStateFromLightingContext();
}

void
UsdMayaGLBatchRenderer::TaskDelegate::_SetLightingStateFromLightingContext()
{
    const GlfSimpleLightVector& lights = _lightingContext->GetLights();

    bool hasNumLightsChanged = false;

    // Insert light Ids into the render index for those that do not yet exist.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId(
            TfStringPrintf("%s/light%d",
                           _rootId.GetText(),
                           (int)_lightIds.size()));
        _lightIds.push_back(lightId);

        // Since we're hardcoded to use HdStRenderDelegate, we expect to have
        // light Sprims.
        TF_VERIFY(GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->light));

        GetRenderIndex().InsertSprim(HdPrimTypeTokens->light, this, lightId);
        hasNumLightsChanged = true;
    }

    // Remove unused light Ids from HdRenderIndex
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->light, _lightIds.back());
        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // invalidate HdLights
    for (size_t i = 0; i < lights.size(); ++i) {
        _ValueCache &cache = _valueCacheMap[_lightIds[i]];
        // store GlfSimpleLight directly.
        cache[HdStLightTokens->params] = VtValue(lights[i]);
        cache[HdStLightTokens->transform] = VtValue();
        cache[HdStLightTokens->shadowParams] = VtValue(HdxShadowParams());
        cache[HdStLightTokens->shadowCollection] = VtValue();

        // Only mark as dirty the parameters to avoid unnecessary invalidation
        // specially marking as dirty lightShadowCollection will trigger
        // a collection dirty on geometry and we don't want that to happen
        // always
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightIds[i], HdStLight::AllDirty);
    }

    // sadly the material also comes from lighting context right now...
    HdxSimpleLightTaskParams taskParams
        = _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                              HdTokens->params);
    taskParams.sceneAmbient = _lightingContext->GetSceneAmbient();
    taskParams.material = _lightingContext->GetMaterial();

    // invalidate HdxSimpleLightTask too
    if (hasNumLightsChanged) {
        _SetValue(_simpleLightTaskId, HdTokens->params, taskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
}

HdTaskSharedPtrVector
UsdMayaGLBatchRenderer::TaskDelegate::GetSetupTasks()
{
    HdTaskSharedPtrVector tasks;

    // lighting
    tasks.push_back(GetRenderIndex().GetTask(_simpleLightTaskId));
    return tasks;
}

HdTaskSharedPtr
UsdMayaGLBatchRenderer::TaskDelegate::GetRenderTask(
    size_t hash,
    RenderParams const &renderParams,
    SdfPathVector const &roots)
{
    // select bucket
    SdfPath renderTaskId;
    if( !TfMapLookup(_renderTaskIdMap, hash, &renderTaskId) )
    {
        // create new render task if not exists
        renderTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("renderTask%zx", hash)));
        _InsertRenderTask(renderTaskId);
        _renderTaskIdMap[hash] = renderTaskId;
    }

    // Update collection in the value cache
    TfToken colName = renderParams.geometryCol;
    HdRprimCollection rprims(colName, renderParams.drawRepr);
    rprims.SetRootPaths(roots);
    rprims.SetRenderTags(renderParams.renderTags);

    // update value cache
    _SetValue(renderTaskId, HdTokens->collection, rprims);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId, HdChangeTracker::DirtyCollection);

    // update render params in the value cache
    HdxRenderTaskParams taskParams =
                _GetValue<HdxRenderTaskParams>(renderTaskId, HdTokens->params);

    // update params
    taskParams.overrideColor         = renderParams.overrideColor;
    taskParams.wireframeColor        = renderParams.wireframeColor;
    taskParams.enableLighting        = renderParams.enableLighting;
    taskParams.enableIdRender        = false;
    taskParams.alphaThreshold        = 0.1;
    taskParams.tessLevel             = 32.0;
    const float tinyThreshold        = 0.9f;
    taskParams.drawingRange          = GfVec2f(tinyThreshold, -1.0f);
    taskParams.depthBiasUseDefault   = true;
    taskParams.depthFunc             = HdCmpFuncLess;
    taskParams.cullStyle             = renderParams.cullStyle;
    taskParams.geomStyle             = HdGeomStylePolygons;
    taskParams.enableHardwareShading = true;

    // note that taskParams.rprims and taskParams.viewport are not updated
    // in this function, and needed to be preserved.

    // store into cache
    _SetValue(renderTaskId, HdTokens->params, taskParams);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,  HdChangeTracker::DirtyParams);

    return GetRenderIndex().GetTask(renderTaskId);
}


UsdMayaGLBatchRenderer::ShapeRenderer *
UsdMayaGLBatchRenderer::GetShapeRenderer(
    const UsdPrim& usdPrim, 
    const SdfPathVector& excludePrimPaths,
    const MDagPath& objPath )
{
    const size_t hash = _ShapeHash( usdPrim, excludePrimPaths, objPath );
    
    // We can get away with this because the spec for std::unordered_map
    // guarantees that data pairs remain valid even if other objects are inserted.
    //
    ShapeRenderer *toReturn = &_shapeRendererMap[ hash ];
    
    if( !toReturn->_batchRenderer )
    {
        // Create a simple hash string to put into a flat SdfPath "hierarchy".
        // This is much faster than more complicated pathing schemes.
        //
        std::string idString = TfStringPrintf("/x%zx", hash);
        
        toReturn->Init(_renderIndex,
                       SdfPath(idString),
                       usdPrim,
                       excludePrimPaths);

        toReturn->_batchRenderer = this;
    }
    
    return toReturn;
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

UsdMayaGLBatchRenderer::UsdMayaGLBatchRenderer()
    : _renderIndex(nullptr)
    , _renderDelegate()
    , _taskDelegate()
    , _intersector()
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    if (!TF_VERIFY(_renderIndex != nullptr)) {
        return;
    }
    _taskDelegate = TaskDelegateSharedPtr(
                          new TaskDelegate(_renderIndex, SdfPath("/mayaTask")));
    _intersector = HdxIntersectorSharedPtr(new HdxIntersector(_renderIndex));


    static MCallbackId sceneUpdateCallbackId = 0;
    if (sceneUpdateCallbackId == 0) {
        sceneUpdateCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kSceneUpdate,
                                       _OnMayaSceneUpdateCallback);
    }
}

UsdMayaGLBatchRenderer::~UsdMayaGLBatchRenderer()
{
    _intersector.reset();
    _taskDelegate.reset();
    delete _renderIndex;
}


/* static */
void UsdMayaGLBatchRenderer::Reset()
{
    if (_sGlobalRendererPtr) {
        MGlobal::displayInfo("Resetting USD Batch Renderer");
    }
    _sGlobalRendererPtr.reset(new UsdMayaGLBatchRenderer());
}

void
UsdMayaGLBatchRenderer::Draw(
    const MDrawRequest& request,
    M3dView &view )
{
    // VP 1.0 Implementation
    //
    MDrawData drawData = request.drawData();
    
    _BatchDrawUserData* batchData =
                static_cast<_BatchDrawUserData*>(drawData.geometry());
    if( !batchData )
        return;
    
    MMatrix projectionMat;
    view.projectionMatrix(projectionMat);
    
    MMatrix modelViewMat;
    view.modelViewMatrix(modelViewMat);

    if( batchData->_bounds )
    {
        px_vp20Utils::RenderBoundingBox(*(batchData->_bounds),
                                        *(batchData->_wireframeColor),
                                        modelViewMat,
                                        projectionMat);
    }
    
    if( batchData->_drawShape && !_renderQueue.empty() )
    {
        MMatrix viewMat( request.matrix().inverse() * modelViewMat );

        unsigned int viewX, viewY, viewWidth, viewHeight;
        view.viewport(viewX, viewY, viewWidth, viewHeight);
        GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

        // Only the first call to this will do anything... After that the batch
        // queue is cleared.
        //
        _RenderBatches( NULL, viewMat, projectionMat, viewport );
    }
    
    // Clean up the _BatchDrawUserData!
    delete batchData;
}

void
UsdMayaGLBatchRenderer::Draw(
    const MHWRender::MDrawContext& context,
    const MUserData *userData)
{
    // VP 2.0 Implementation
    //
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if( !theRenderer || !theRenderer->drawAPIIsOpenGL() )
        return;

    const _BatchDrawUserData* batchData = static_cast<const _BatchDrawUserData*>(userData);
    if( !batchData )
        return;
    
    MStatus status;

    MMatrix projectionMat = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
    
    if( batchData->_bounds )
    {
        MMatrix worldViewMat = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx, &status);

        px_vp20Utils::RenderBoundingBox(*(batchData->_bounds),
                                        *(batchData->_wireframeColor),
                                        worldViewMat,
                                        projectionMat);
    }
    
    if( batchData->_drawShape && !_renderQueue.empty() )
    {
        MMatrix viewMat = context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);

        // Extract camera settings from maya view
        int viewX, viewY, viewWidth, viewHeight;
        context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);
        GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

        // Only the first call to this will do anything... After that the batch
        // queue is cleared.
        //
        _RenderBatches( &context, viewMat, projectionMat, viewport );
    }
}

size_t
UsdMayaGLBatchRenderer::_ShapeHash(
    const UsdPrim& usdPrim, 
    const SdfPathVector& excludePrimPaths,
    const MDagPath& objPath )
{
    size_t hash( MObjectHandle(objPath.transform()).hashCode() );
    boost::hash_combine( hash, usdPrim );
    boost::hash_combine( hash, excludePrimPaths );

    return hash;
}

void
UsdMayaGLBatchRenderer::_QueueShapeForDraw(
    const SdfPath& sharedId,
    const RenderParams &params )
{
    _QueuePathForDraw( sharedId, params );
}

void
UsdMayaGLBatchRenderer::_QueuePathForDraw(
    const SdfPath& sharedId,
    const RenderParams &params )
{
    size_t paramKey = params.Hash();
    
    auto renderSetIter = _renderQueue.find( paramKey );
    if( renderSetIter == _renderQueue.end() )
    {
        // If we had no _SdfPathSet for this particular RenderParam combination,
        // create a new one.
        _renderQueue[paramKey] = _RenderParamSet( params, _SdfPathSet( {sharedId} ) );
    }
    else
    {
        _SdfPathSet &renderPaths = renderSetIter->second.second;
        renderPaths.insert( sharedId );
    }
}


const HdxIntersector::Hit * 
UsdMayaGLBatchRenderer::_GetHitInfo(
    M3dView& view,
    unsigned int pickResolution,
    bool singleSelection,
    const SdfPath& sharedId,
    const GfMatrix4d &localToWorldSpace)
{
    // Guard against user clicking in viewer before renderer is setup
    if( !_renderIndex )
        return NULL;

    // Selection only occurs once per display refresh, with all usd objects
    // simulataneously. If the selectQueue is not empty, that means that
    // a refresh has occurred, and we need to perform a new selection operation.
    
    if( !_selectQueue.empty() )
    {
        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "____________ SELECTION STAGE START ______________ (singleSelect = %d)\n",
            singleSelection );

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
        qparams.alphaThreshold = 0.1;
        
        _selectResults.clear();

        for( const auto &renderSetIter : _selectQueue )
        {
            const RenderParams &renderParams = renderSetIter.second.first;
            const _SdfPathSet &renderPaths = renderSetIter.second.second;
            SdfPathVector roots(renderPaths.begin(), renderPaths.end());
            
            TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                    "--- pickQueue, batch %zx, size %zu\n",
                    renderSetIter.first, renderPaths.size());
            
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
            if( !r ) {
                continue;
            }
            
            if( singleSelection )
            {
                hits.resize(1);
                if( !result.ResolveNearest(&hits.front()) )
                    continue;
            }
            else if( !result.ResolveAll(&hits) )
            {
                continue;
            }

            for (const HdxIntersector::Hit& hit : hits) {
                auto itIfExists =
                    _selectResults.insert(
                        std::pair<SdfPath, HdxIntersector::Hit>(hit.delegateId, hit));
                
                const bool &inserted = itIfExists.second;
                if( inserted )
                    continue;
                                
                HdxIntersector::Hit& existingHit = itIfExists.first->second;
                if( hit.ndcDepth < existingHit.ndcDepth )
                    existingHit = hit;
            }
        }
        
        if( singleSelection && _selectResults.size()>1 )
        {
            TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                    "!!! multiple singleSel hits found: %zu\n",
                    _selectResults.size());
            
            auto minIt=_selectResults.begin();
            for( auto curIt=minIt; curIt!=_selectResults.end(); curIt++ )
            {
                const HdxIntersector::Hit& curHit = curIt->second;
                const HdxIntersector::Hit& minHit = minIt->second;
                if( curHit.ndcDepth < minHit.ndcDepth )
                    minIt = curIt;
            }
            
            if( minIt!=_selectResults.begin() )
                _selectResults.erase(_selectResults.begin(),minIt);
            minIt++;
            if( minIt!=_selectResults.end() )
                _selectResults.erase(minIt,_selectResults.end());
        }
        
        if( TfDebug::IsEnabled(PXRUSDMAYAGL_QUEUE_INFO) )
        {
            for ( const auto &selectPair : _selectResults)
            {
                const SdfPath& path = selectPair.first;
                const HdxIntersector::Hit& hit = selectPair.second;
                cout << "NEW HIT: " << path << endl;
                cout << "\tdelegateId: " << hit.delegateId << endl;
                cout << "\tobjectId: " << hit.objectId << endl;
                cout << "\tndcDepth: " << hit.ndcDepth << endl;
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
        
        if( _selectResults.empty() )
            view.scheduleRefresh();
        
        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "^^^^^^^^^^^^ SELECTION STAGE FINISH ^^^^^^^^^^^^^\n");
    }
    
    return TfMapLookupPtr( _selectResults, sharedId );
}


void
UsdMayaGLBatchRenderer::_RenderBatches( 
    const MHWRender::MDrawContext* vp2Context,
    const MMatrix& viewMat,
    const MMatrix& projectionMat,
    const GfVec4d& viewport )
{
    if( _renderQueue.empty() )
        return;
    
    if( !_populateQueue.empty() )
    {
        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "____________ POPULATE STAGE START ______________ (%zu)\n",_populateQueue.size());

        std::vector<UsdImagingDelegate*> delegates;
        UsdPrimVector rootPrims;
        std::vector<SdfPathVector> excludedPrimPaths;
        std::vector<SdfPathVector> invisedPrimPaths;
        
        for( ShapeRenderer *shapeRenderer : _populateQueue )
        {
            delegates.push_back(shapeRenderer->_delegate.get());
            rootPrims.push_back(shapeRenderer->_rootPrim);
            excludedPrimPaths.push_back(shapeRenderer->_excludedPaths);
            invisedPrimPaths.push_back(SdfPathVector());
            
            shapeRenderer->_isPopulated = true;
        }
        
        UsdImagingDelegate::Populate( delegates,
                                      rootPrims,
                                      excludedPrimPaths,
                                      invisedPrimPaths );
        
        _populateQueue.clear();

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
            "^^^^^^^^^^^^ POPULATE STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n",_populateQueue.size());
    }
    
    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "____________ RENDER STAGE START ______________ (%zu)\n",_renderQueue.size());

    // A new display refresh signifies that the cached selection data is no
    // longer valid.
    _selectQueue.clear();
    _selectResults.clear();

    // We've already populated with all the selection info we need.  We Reset
    // and the first call to GetSoftSelectHelper in the next render pass will
    // re-populate it.
    _softSelectHelper.Reset();
    
    GfMatrix4d modelViewMatrix(viewMat.matrix);
    GfMatrix4d projectionMatrix(projectionMat.matrix);

    _taskDelegate->SetCameraState(modelViewMatrix, projectionMatrix, viewport);

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
        _taskDelegate->SetLightingStateFromVP1(viewMat);
    }
    
    // The legacy viewport does not support color management,
    // so we roll our own gamma correction by GL means (only in
    // non-highlight mode)
    bool gammaCorrect = !vp2Context;

    if( gammaCorrect )
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render task setup
    HdTaskSharedPtrVector tasks = _taskDelegate->GetSetupTasks(); // lighting etc

    for( const auto &renderSetIter : _renderQueue )
    {
        size_t hash = renderSetIter.first;
        const RenderParams &params = renderSetIter.second.first;
        const _SdfPathSet &renderPaths = renderSetIter.second.second;

        TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
                "*** renderQueue, batch %zx, size %zu\n",
                renderSetIter.first, renderPaths.size() );

        SdfPathVector roots(renderPaths.begin(), renderPaths.end());
        tasks.push_back(_taskDelegate->GetRenderTask(hash, params, roots));
    }

    _hdEngine.Execute(*_renderIndex, tasks);
    
    if( gammaCorrect )
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);

    glPopAttrib(); // GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT
    
    // Selection is based on what we have last rendered to the display. The
    // selection queue is cleared above, so this has the effect of resetting
    // the render queue and prepping the selection queue without any
    // significant memory hit.
    _renderQueue.swap( _selectQueue );
    
    TF_DEBUG(PXRUSDMAYAGL_QUEUE_INFO).Msg(
        "^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^ (%zu)\n",_renderQueue.size());
}


PXR_NAMESPACE_CLOSE_SCOPE
