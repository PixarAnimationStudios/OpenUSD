//
// Copyright 2018 Pixar
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
#include "pxrUsdMayaGL/shapeAdapter.h"

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/softSelectHelper.h"
#include "usdMaya/proxyShape.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <boost/functional/hash.hpp>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((RenderGuidesTag, "render"))
);


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE,
        "Report Maya Hydra shape adapter lifecycle events.");
}


PxrMayaHdShapeAdapter::PxrMayaHdShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Constructing PxrMayaHdShapeAdapter: %p\n",
        this);
}

/* virtual */
PxrMayaHdShapeAdapter::~PxrMayaHdShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Destructing PxrMayaHdShapeAdapter: %p\n",
        this);
}

bool
PxrMayaHdShapeAdapter::_Init(HdRenderIndex* renderIndex)
{
    if (!TF_VERIFY(renderIndex,
                   "Cannot initialize shape adapter with invalid HdRenderIndex")) {
        return false;
    }

    const SdfPath delegatePrefix =
        UsdMayaGLBatchRenderer::GetInstance().GetDelegatePrefix(_isViewport2);

    // Create a simple "name" for this shape adapter to insert into the batch
    // renderer's SdfPath hierarchy.
    //
    // XXX: For as long as we're using the MAYA_VP2_USE_VP1_SELECTION
    // environment variable, we need to be able to pass responsibility back and
    // forth between the MPxDrawOverride's shape adapter for drawing and the
    // MPxSurfaceShapeUI's shape adapter for selection. This requires both
    // shape adapters to have the same "name", which forces us to build it
    // from data on the shape that will be common to both classes, as we do
    // below. When we remove MAYA_VP2_USE_VP1_SELECTION and can trust that a
    // single shape adapter handles both drawing and selection, we can do
    // something even simpler instead like using the shape adapter's memory
    // address as the "name".
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _rootPrim);
    boost::hash_combine(shapeHash, _excludedPrimPaths);

    // We prepend the Maya type name to the beginning of the delegate name to
    // ensure that there are no name collisions between shape adapters of
    // shapes with different Maya types.
    const TfToken delegateName(
        TfStringPrintf("%s_%zx",
                       PxrUsdMayaProxyShapeTokens->MayaTypeName.GetText(),
                       shapeHash));

    const SdfPath delegateId = delegatePrefix.AppendChild(delegateName);

    if (_delegate &&
            delegateId == GetDelegateID() &&
            renderIndex == &_delegate->GetRenderIndex()) {
        // The delegate's current ID matches the delegate ID we computed and
        // the render index matches, so it must be up to date already.
        return true;
    }

    const TfToken collectionName(_shapeDagPath.fullPathName().asChar());

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Initializing PxrMayaHdShapeAdapter: %p\n"
        "    collection name: %s\n"
        "    delegateId     : %s\n",
        this,
        collectionName.GetText(),
        delegateId.GetText());

    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateId));
    if (!TF_VERIFY(_delegate,
                  "Failed to create shape adapter delegate for shape %s",
                  _shapeDagPath.fullPathName().asChar())) {
        return false;
    }

    if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
            "    Populating delegate:\n"
            "        rootPrim         : %s\n"
            "        excludedPrimPaths: ",
            _rootPrim.GetPath().GetText());
        for (const SdfPath& primPath : _excludedPrimPaths) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "%s ",
                primPath.GetText());
        }
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg("\n");
    }

    _delegate->Populate(_rootPrim, _excludedPrimPaths, SdfPathVector());

    if (collectionName != _rprimCollection.GetName()) {
        _rprimCollection.SetName(collectionName);
        renderIndex->GetChangeTracker().AddCollection(_rprimCollection.GetName());
    }

    _rprimCollection.SetReprName(HdTokens->refined);
    _rprimCollection.SetRootPath(delegateId);

    return true;
}

// Helper function that converts M3dView::DisplayStyle (legacy viewport) into
// MHWRender::MFrameContext::DisplayStyle (Viewport 2.0).
//
// In the legacy viewport, the M3dView can be in exactly one displayStyle
// whereas Viewport 2.0's displayStyle is a bitmask of potentially multiple
// styles. To translate from the legacy viewport to Viewport 2.0, we simply
// bitwise-OR the single legacy viewport displayStyle into an empty mask.
static inline
unsigned int
_ToMFrameContextDisplayStyle(const M3dView::DisplayStyle legacyDisplayStyle)
{
    unsigned int displayStyle = 0u;

    switch (legacyDisplayStyle) {
        case M3dView::kBoundingBox:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kBoundingBox;
            break;
        case M3dView::kFlatShaded:
// MHWRender::MFrameContext::DisplayStyle::kFlatShaded is missing in Maya 2015
// and earlier. For those versions of Maya, fall through to kGouraudShaded.
#if MAYA_API_VERSION >= 201600
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
            break;
#endif
        case M3dView::kGouraudShaded:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kGouraudShaded;
            break;
        case M3dView::kWireFrame:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kWireFrame;
            break;
        case M3dView::kPoints:
            // Not supported.
            break;
    }

    return displayStyle;
}

// Helper function that converts M3dView::DisplayStatus (legacy viewport) into
// MHWRender::DisplayStatus (Viewport 2.0).
static inline
MHWRender::DisplayStatus
_ToMHWRenderDisplayStatus(const M3dView::DisplayStatus legacyDisplayStatus)
{
    // These enums are equivalent, but statically checking just in case.
    static_assert(
        ((int)M3dView::kActive == (int)MHWRender::kActive) &&
        ((int)M3dView::kLive == (int)MHWRender::kLive) &&
        ((int)M3dView::kDormant == (int)MHWRender::kDormant) &&
        ((int)M3dView::kInvisible == (int)MHWRender::kInvisible) &&
        ((int)M3dView::kHilite == (int)MHWRender::kHilite) &&
        ((int)M3dView::kTemplate == (int)MHWRender::kTemplate) &&
        ((int)M3dView::kActiveTemplate == (int)MHWRender::kActiveTemplate) &&
        ((int)M3dView::kActiveComponent == (int)MHWRender::kActiveComponent) &&
        ((int)M3dView::kLead == (int)MHWRender::kLead) &&
        ((int)M3dView::kIntermediateObject == (int)MHWRender::kIntermediateObject) &&
        ((int)M3dView::kActiveAffected == (int)MHWRender::kActiveAffected) &&
        ((int)M3dView::kNoStatus == (int)MHWRender::kNoStatus),
            "M3dView::DisplayStatus == MHWRender::DisplayStatus");

    return MHWRender::DisplayStatus((int)legacyDisplayStatus);
}

/* static */
bool
PxrMayaHdShapeAdapter::_GetWireframeColor(
        const MHWRender::DisplayStatus displayStatus,
        const MDagPath& shapeDagPath,
        MColor* mayaWireColor)
{
    // Dormant objects may be included in a soft selection.
    if (displayStatus == MHWRender::kDormant) {
        const UsdMayaGLSoftSelectHelper& softSelectHelper =
            UsdMayaGLBatchRenderer::GetInstance().GetSoftSelectHelper();
        return softSelectHelper.GetFalloffColor(shapeDagPath, mayaWireColor);
    }
    else if ((displayStatus == MHWRender::kActive) ||
             (displayStatus == MHWRender::kLead) ||
             (displayStatus == MHWRender::kHilite)) {
        *mayaWireColor =
            MHWRender::MGeometryUtilities::wireframeColor(shapeDagPath);
        return true;
    }

    return false;
}

bool
PxrMayaHdShapeAdapter::Sync(
        MPxSurfaceShape* surfaceShape,
        const M3dView::DisplayStyle legacyDisplayStyle,
        const M3dView::DisplayStatus legacyDisplayStatus)
{
    // Legacy viewport implementation.
    _isViewport2 = false;

    const unsigned int displayStyle =
        _ToMFrameContextDisplayStyle(legacyDisplayStyle);
    const MHWRender::DisplayStatus displayStatus =
        _ToMHWRenderDisplayStatus(legacyDisplayStatus);

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Synchronizing PxrMayaHdShapeAdapter for legacy viewport: %p\n",
        this);

    const bool success = _Sync(surfaceShape, displayStyle, displayStatus);

    if (success) {
        // The legacy viewport does not support color management, so we roll
        // our own gamma correction via framebuffer effect. But that means we
        // need to pre-linearize the wireframe color from Maya.
        //
        // The default value for wireframeColor is 0.0f for all four values and
        // if we need a wireframe color, we expect _Sync() to have set the
        // values and put 1.0f in for alpha, so inspect the alpha value to
        // determine whether we need to linearize rather than calling
        // _GetWireframeColor() again.
        if (_renderParams.wireframeColor[3] > 0.0f) {
            _renderParams.wireframeColor[3] = 1.0f;
            _renderParams.wireframeColor =
                GfConvertDisplayToLinear(_renderParams.wireframeColor);
        }
    }

    return success;
}

bool
PxrMayaHdShapeAdapter::Sync(
        MPxSurfaceShape* surfaceShape,
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus)
{
    // Viewport 2.0 implementation.
    _isViewport2 = true;

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Synchronizing PxrMayaHdShapeAdapter for Viewport 2.0: %p\n",
        this);

    return _Sync(surfaceShape, displayStyle, displayStatus);
}

/* virtual */
bool
PxrMayaHdShapeAdapter::_Sync(
        MPxSurfaceShape* surfaceShape,
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus)
{
    UsdMayaProxyShape* usdProxyShape =
        dynamic_cast<UsdMayaProxyShape*>(surfaceShape);
    if (!usdProxyShape) {
        TF_WARN("Failed to get UsdMayaProxyShape.");
        return false;
    }

    UsdPrim usdPrim;
    SdfPathVector excludedPrimPaths;
    int refineLevel;
    UsdTimeCode timeCode;
    bool showGuides;
    bool showRenderGuides;
    bool tint;
    GfVec4f tintColor;
    if (!usdProxyShape->GetAllRenderAttributes(&usdPrim,
                                               &excludedPrimPaths,
                                               &refineLevel,
                                               &timeCode,
                                               &showGuides,
                                               &showRenderGuides,
                                               &tint,
                                               &tintColor)) {
        TF_WARN("Failed to get render attributes for UsdMayaProxyShape.");
        return false;
    }

    // Check for updates to the shape or changes in the batch renderer that
    // require us to re-initialize the shape adapter.
    MStatus status;
    const MFnDagNode dagNodeFn(usdProxyShape->thisMObject(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    MDagPath shapeDagPath;
    status = dagNodeFn.getPath(shapeDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);

    HdRenderIndex* renderIndex =
        UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();
    if (!(shapeDagPath == _shapeDagPath) ||
            usdPrim != _rootPrim ||
            excludedPrimPaths != _excludedPrimPaths ||
            !_delegate ||
            renderIndex != &_delegate->GetRenderIndex()) {
        _shapeDagPath = shapeDagPath;
        _rootPrim = usdPrim;
        _excludedPrimPaths = excludedPrimPaths;

        if (!_Init(renderIndex)) {
            return false;
        }
    }

    // Reset _renderParams to the defaults.
    PxrMayaHdRenderParams renderParams;
    _renderParams = renderParams;

    if (tint) {
        _renderParams.overrideColor = tintColor;
    }

    // XXX Not yet adding ability to turn off display of proxy geometry, but
    // we should at some point, as in usdview.
    TfTokenVector renderTags;
    renderTags.push_back(HdTokens->geometry);
    renderTags.push_back(HdTokens->proxy);
    if (showGuides) {
        renderTags.push_back(HdTokens->guide);
    }
    if (showRenderGuides) {
        renderTags.push_back(_tokens->RenderGuidesTag);
    }

    if (_rprimCollection.GetRenderTags() != renderTags) {
        _rprimCollection.SetRenderTags(renderTags);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
            "    Render tags changed: %s\n"
            "        Marking collection dirty: %s\n",
            TfStringJoin(renderTags.begin(), renderTags.end()).c_str(),
            _rprimCollection.GetName().GetText());

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    const MMatrix transform = _shapeDagPath.inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        _rootXform = GfMatrix4d(transform.matrix);
        _delegate->SetRootTransform(_rootXform);
    }

    _delegate->SetRefineLevelFallback(refineLevel);

    // Will only react if time actually changes.
    _delegate->SetTime(timeCode);

    _delegate->SetRootCompensation(_rootPrim.GetPath());

    _drawShape = true;
    _drawBoundingBox =
        (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    MColor mayaWireframeColor;
    const bool needsWire = _GetWireframeColor(displayStatus,
                                              _shapeDagPath,
                                              &mayaWireframeColor);
    if (needsWire) {
        _renderParams.wireframeColor = GfVec4f(mayaWireframeColor.r,
                                               mayaWireframeColor.g,
                                               mayaWireframeColor.b,
                                               1.0f);
    }

    TfToken reprName;

    // Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for
    // whatever reason...
    const bool flatShaded =
#if MAYA_API_VERSION >= 201600
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
        false;
#endif

    if (flatShaded) {
        if (needsWire) {
            reprName = HdTokens->wireOnSurf;
        } else {
            reprName = HdTokens->hull;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded)
    {
        if (needsWire || (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)) {
            reprName = HdTokens->refinedWireOnSurf;
        } else {
            reprName = HdTokens->refined;
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
    {
        reprName = HdTokens->refinedWire;
        _renderParams.enableLighting = false;
    }
    else
    {
        _drawShape = false;
    }

    if (_delegate->GetRootVisibility() != _drawShape) {
        _delegate->SetRootVisibility(_drawShape);
    }

    if (_rprimCollection.GetReprName() != reprName) {
        _rprimCollection.SetReprName(reprName);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "    Repr name changed: %s\n"
                "        Marking collection dirty: %s\n",
                reprName.GetText(),
                _rprimCollection.GetName().GetText());

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    // Maya 2016 SP2 lacks MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling
    // for whatever reason...
    _renderParams.cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        _renderParams.cullStyle = HdCullStyleBackUnlessDoubleSided;
    }
#endif

    return true;
}

bool
PxrMayaHdShapeAdapter::UpdateVisibility()
{
    MStatus status;
    const MHWRender::DisplayStatus displayStatus =
        MHWRender::MGeometryUtilities::displayStatus(_shapeDagPath, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const bool isVisible = (displayStatus != MHWRender::kInvisible);

    if (_delegate && _delegate->GetRootVisibility() != isVisible) {
        _delegate->SetRootVisibility(isVisible);
        return true;
    }

    return false;
}

PxrMayaHdRenderParams
PxrMayaHdShapeAdapter::GetRenderParams(
        bool* drawShape,
        bool* drawBoundingBox) const
{
    if (drawShape) {
        *drawShape = _drawShape;
    }

    if (drawBoundingBox) {
        *drawBoundingBox = _drawBoundingBox;
    }

    return _renderParams;
}

void
PxrMayaHdShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;

    if (_delegate) {
        _delegate->SetRootTransform(_rootXform);
    }
}

const SdfPath&
PxrMayaHdShapeAdapter::GetDelegateID() const
{
    if (_delegate) {
        return _delegate->GetDelegateID();
    }

    return SdfPath::EmptyPath();
}

bool
PxrMayaHdShapeAdapter::IsViewport2() const
{
    return _isViewport2;
}


PXR_NAMESPACE_CLOSE_SCOPE
