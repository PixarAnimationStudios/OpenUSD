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


PxrMayaHdShapeAdapter::PxrMayaHdShapeAdapter() :
        _isPopulated(false)
{
}

/* virtual */
PxrMayaHdShapeAdapter::~PxrMayaHdShapeAdapter()
{
}

void
PxrMayaHdShapeAdapter::Init(HdRenderIndex* renderIndex)
{
    // Create a simple string to put into a flat SdfPath "hierarchy". This is
    // much faster than more complicated pathing schemes.
    //
    // XXX: For as long as we're using the MAYA_VP2_USE_VP1_SELECTION
    // environment variable, we need to be able to pass responsibility back and
    // forth between the MPxDrawOverride's shape adapter for drawing and the
    // MPxSurfaceShapeUI's shape adapter for selection. This requires both
    // shape adapters to have the same delegate ID, which forces us to build it
    // from data on the shape that will be common to both classes, as we do
    // below. When we remove MAYA_VP2_USE_VP1_SELECTION and can trust that a
    // single shape adapter handles both drawing and selection, we can do
    // something even simpler instead like using the shape adapter's memory
    // address as the ID:
    //
    // const std::string idString = TfStringPrintf("/PxrMayaHdShapeAdapter_%p",
    //                                             this);
    //
    // Note that this also means that the properties used to compute the
    // delegateId must be populated *before* this method is called, and
    // therefore also before UsdMayaGLBatchRenderer::AddShapeAdapter() is
    // called.
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _rootPrim);
    boost::hash_combine(shapeHash, _excludedPrimPaths);

    const std::string idString = TfStringPrintf("/x%zx", shapeHash);

    const SdfPath delegateId(idString);

    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateId));
    _isPopulated = false;

    _rprimCollection.SetName(TfToken(_shapeDagPath.fullPathName().asChar()));
    _rprimCollection.SetReprName(HdTokens->refined);
    _rprimCollection.SetRootPath(delegateId);

    renderIndex->GetChangeTracker().AddCollection(_rprimCollection.GetName());
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

bool
PxrMayaHdShapeAdapter::_GetWireframeColor(
        const MHWRender::DisplayStatus displayStatus,
        MColor* mayaWireColor)
{
    // Dormant objects may be included in a soft selection.
    if (displayStatus == MHWRender::kDormant) {
        const UsdMayaGLSoftSelectHelper& softSelectHelper =
            UsdMayaGLBatchRenderer::GetInstance().GetSoftSelectHelper();
        return softSelectHelper.GetFalloffColor(_shapeDagPath, mayaWireColor);
    }
    else if ((displayStatus == MHWRender::kActive) ||
             (displayStatus == MHWRender::kLead) ||
             (displayStatus == MHWRender::kHilite)) {
        *mayaWireColor =
            MHWRender::MGeometryUtilities::wireframeColor(_shapeDagPath);
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
    const unsigned int displayStyle =
        _ToMFrameContextDisplayStyle(legacyDisplayStyle);
    const MHWRender::DisplayStatus displayStatus =
        _ToMHWRenderDisplayStatus(legacyDisplayStatus);

    const bool success = Sync(surfaceShape, displayStyle, displayStatus);

    if (success) {
        // The legacy viewport does not support color management, so we roll
        // our own gamma correction via framebuffer effect. But that means we
        // need to pre-linearize the wireframe color from Maya.
        //
        // The default value for wireframeColor is 0.0f for all four values and
        // if we need a wireframe color, we expect Sync() to have set the
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
    UsdMayaProxyShape* usdProxyShape =
        dynamic_cast<UsdMayaProxyShape*>(surfaceShape);
    if (!usdProxyShape) {
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
        return false;
    }

    if (_rootPrim != usdPrim) {
        _rootPrim = usdPrim;
        _isPopulated = false;
    }
    if (_excludedPrimPaths != excludedPrimPaths) {
        _excludedPrimPaths = excludedPrimPaths;
        _isPopulated = false;
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

        if (_delegate) {
            _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
                _rprimCollection.GetName());
        }
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
        _delegate->SetTime(timeCode);

        _delegate->SetRootCompensation(_rootPrim.GetPath());

        if (!_isPopulated) {
            _delegate->Populate(_rootPrim, _excludedPrimPaths, SdfPathVector());
            _isPopulated = true;
        }
    }

    _drawShape = true;
    _drawBoundingBox =
        (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    MColor mayaWireframeColor;
    const bool needsWire = _GetWireframeColor(displayStatus,
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

    if (_rprimCollection.GetReprName() != reprName) {
        _rprimCollection.SetReprName(reprName);

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

PxrMayaHdRenderParams
PxrMayaHdShapeAdapter::GetRenderParams(bool* drawShape, bool* drawBoundingBox)
{
    if (drawShape) {
        *drawShape = _drawShape;
    }

    if (drawBoundingBox) {
        *drawBoundingBox = _drawBoundingBox;
    }

    return _renderParams;
}

const SdfPath&
PxrMayaHdShapeAdapter::GetDelegateID() const
{
    if (_delegate) {
        return _delegate->GetDelegateID();
    }

    return SdfPath::EmptyPath();
}


PXR_NAMESPACE_CLOSE_SCOPE
