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
#include "pxrUsdMayaGL/debugCodes.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/softSelectHelper.h"
#include "pxrUsdMayaGL/userData.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectionList.h>
#include <maya/MUserData.h>


PXR_NAMESPACE_OPEN_SCOPE


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

/* virtual */
bool
PxrMayaHdShapeAdapter::Sync(
        const MDagPath& shapeDagPath,
        const M3dView::DisplayStyle legacyDisplayStyle,
        const M3dView::DisplayStatus legacyDisplayStatus)
{
    // Legacy viewport implementation.
    _isViewport2 = false;

    UsdMayaGLBatchRenderer::GetInstance().StartBatchingFrameDiagnostics();

    const unsigned int displayStyle =
        _ToMFrameContextDisplayStyle(legacyDisplayStyle);
    const MHWRender::DisplayStatus displayStatus =
        _ToMHWRenderDisplayStatus(legacyDisplayStatus);

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Synchronizing PxrMayaHdShapeAdapter for legacy viewport: %p\n",
        this);

    const bool success = _Sync(shapeDagPath, displayStyle, displayStatus);

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

/* virtual */
bool
PxrMayaHdShapeAdapter::Sync(
        const MDagPath& shapeDagPath,
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus)
{
    // Viewport 2.0 implementation.
    _isViewport2 = true;

    UsdMayaGLBatchRenderer::GetInstance().StartBatchingFrameDiagnostics();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Synchronizing PxrMayaHdShapeAdapter for Viewport 2.0: %p\n",
        this);

    return _Sync(shapeDagPath, displayStyle, displayStatus);
}

/* virtual */
bool
PxrMayaHdShapeAdapter::UpdateVisibility(const M3dView* view)
{
    return false;
}

/* virtual */
bool
PxrMayaHdShapeAdapter::IsVisible() const
{
    return false;
}

/* virtual */
void
PxrMayaHdShapeAdapter::GetMayaUserData(
        MPxSurfaceShapeUI* shapeUI,
        MDrawRequest& drawRequest,
        const MBoundingBox* boundingBox)
{
    // Legacy viewport implementation.

    // The legacy viewport never has an old MUserData we can reuse.
    MUserData* userData = GetMayaUserData(nullptr, boundingBox);

    // Note that the legacy viewport does not manage the data allocated in the
    // MDrawData object, so the batch renderer deletes the MUserData object at
    // the end of a legacy viewport Draw() call.
    MDrawData drawData;
    shapeUI->getDrawData(userData, drawData);

    drawRequest.setDrawData(drawData);
}

/* virtual */
PxrMayaHdUserData*
PxrMayaHdShapeAdapter::GetMayaUserData(
        MUserData* oldData,
        const MBoundingBox* boundingBox)
{
    // Viewport 2.0 implementation (also called by legacy viewport
    // implementation).
    //
    // Our PxrMayaHdUserData can be used to signify whether we are requesting a
    // shape to be rendered, a bounding box, both, or neither.
    //
    // In the Viewport 2.0 prepareForDraw() usage, any MUserData object passed
    // into the function will be deleted by Maya. In the legacy viewport usage,
    // the object gets deleted at the end of a legacy viewport Draw() call.

    if (!_drawShape && !boundingBox) {
        return nullptr;
    }

    PxrMayaHdUserData* newData = dynamic_cast<PxrMayaHdUserData*>(oldData);
    if (!newData) {
        newData = new PxrMayaHdUserData();
    }

    // Internally, the shape adapter keeps track of whether its shape is being
    // drawn for managing visibility, but otherwise most Hydra-imaged shapes
    // should not be drawing themselves. The pxrHdImagingShape will take care
    // of batching up the drawing of all of the shapes, so we specify in the
    // Maya user data that the shape should *not* draw by default. The
    // pxrHdImagingShape bypasses this and sets drawShape to true.
    // We handle this similarly in GetRenderParams() below.
    newData->drawShape = false;

    if (boundingBox) {
        newData->boundingBox.reset(new MBoundingBox(*boundingBox));
        newData->wireframeColor.reset(new GfVec4f(_renderParams.wireframeColor));
    } else {
        newData->boundingBox.reset();
        newData->wireframeColor.reset();
    }

    return newData;
}

/* virtual */
HdReprSelector
PxrMayaHdShapeAdapter::GetReprSelectorForDisplayState(
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus) const
{
    HdReprSelector reprSelector;

    const bool boundingBoxStyle =
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox;

    if (boundingBoxStyle) {
        // We don't currently use Hydra to draw bounding boxes, so we return an
        // empty repr selector here. Also, Maya seems to ignore most other
        // DisplayStyle bits when the viewport is in the kBoundingBox display
        // style anyway, and it just changes the color of the bounding box on
        // selection rather than adding in the wireframe like it does for
        // shaded display styles. So if we eventually do end up using Hydra for
        // bounding boxes, we could just return the appropriate repr here.
        return reprSelector;
    }

    const bool shadeActiveOnlyStyle =
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kShadeActiveOnly;

    const bool isActive =
        (displayStatus == MHWRender::DisplayStatus::kActive) ||
        (displayStatus == MHWRender::DisplayStatus::kHilite) ||
        (displayStatus == MHWRender::DisplayStatus::kActiveTemplate) ||
        (displayStatus == MHWRender::DisplayStatus::kActiveComponent) ||
        (displayStatus == MHWRender::DisplayStatus::kLead);

    const bool wireframeStyle =
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame;

    // The kFlatShaded display style was introduced in Maya 2016.
    const bool flatShadedStyle =
#if MAYA_API_VERSION >= 201600
        displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
#else
        false;
#endif

    if (flatShadedStyle) {
        if (!shadeActiveOnlyStyle || isActive) {
            if (wireframeStyle) {
                reprSelector = HdReprSelector(HdReprTokens->wireOnSurf);
            } else {
                reprSelector = HdReprSelector(HdReprTokens->hull);
            }
        } else {
            // We're in shadeActiveOnly mode but this shape is not active.
            reprSelector = HdReprSelector(HdReprTokens->wire);
        }
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded) {
        if (!shadeActiveOnlyStyle || isActive) {
            if (wireframeStyle) {
                reprSelector = HdReprSelector(HdReprTokens->refinedWireOnSurf);
            } else {
                reprSelector = HdReprSelector(HdReprTokens->refined);
            }
        } else {
            // We're in shadeActiveOnly mode but this shape is not active.
            reprSelector = HdReprSelector(HdReprTokens->refinedWire);
        }
    }
    else if (wireframeStyle) {
        reprSelector = HdReprSelector(HdReprTokens->refinedWire);
    }
    else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kTwoSidedLighting) {
        // The UV editor uses the kTwoSidedLighting displayStyle.
        //
        // For now, to prevent objects from completely disappearing, we just
        // treat it similarly to kGouraudShaded.
        reprSelector = HdReprSelector(HdReprTokens->refined);
    }

    return reprSelector;
}

/* virtual */
PxrMayaHdRenderParams
PxrMayaHdShapeAdapter::GetRenderParams(
        bool* drawShape,
        bool* drawBoundingBox) const
{
    if (drawShape) {
        // Internally, the shape adapter keeps track of whether its shape is
        // being drawn for managing visibility, but otherwise most Hydra-imaged
        // shapes should not be drawing themselves. The pxrHdImagingShape will
        // take care of batching up the drawing of all of the shapes, so for
        // the purposes of render params, we set drawShape to false by default.
        // The pxrHdImagingShape bypasses this and sets drawShape to true.
        // We handle this similarly in GetMayaUserData() above.
        *drawShape = false;
    }

    if (drawBoundingBox) {
        *drawBoundingBox = _drawBoundingBox;
    }

    return _renderParams;
}

/* virtual */
const HdRprimCollection&
PxrMayaHdShapeAdapter::GetRprimCollection() const
{
    return _rprimCollection;
}

/* virtual */
const GfMatrix4d&
PxrMayaHdShapeAdapter::GetRootXform() const
{
    return _rootXform;
}

/* virtual */
void
PxrMayaHdShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;
}

/* virtual */
const SdfPath&
PxrMayaHdShapeAdapter::GetDelegateID() const
{
    return SdfPath::EmptyPath();
}

/* virtual */
const MDagPath&
PxrMayaHdShapeAdapter::GetDagPath() const
{
    return _shapeDagPath;
}

/* virtual */
bool
PxrMayaHdShapeAdapter::IsViewport2() const
{
    return _isViewport2;
}

/* static */
bool
PxrMayaHdShapeAdapter::_GetWireframeColor(
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus,
        const MDagPath& shapeDagPath,
        MColor* mayaWireColor)
{
    bool useWireframeColor = false;

    // Dormant objects may be included in a soft selection.
    if (displayStatus == MHWRender::kDormant) {
        auto& batchRenderer = UsdMayaGLBatchRenderer::GetInstance();
        if (batchRenderer.GetObjectSoftSelectEnabled()) {
            const UsdMayaGLSoftSelectHelper& softSelectHelper =
                UsdMayaGLBatchRenderer::GetInstance().GetSoftSelectHelper();
            useWireframeColor = softSelectHelper.GetFalloffColor(shapeDagPath,
                                                                 mayaWireColor);
        }
    }

    // If the object isn't included in a soft selection, just ask Maya for the
    // wireframe color.
    if (!useWireframeColor && mayaWireColor != nullptr) {
        *mayaWireColor =
            MHWRender::MGeometryUtilities::wireframeColor(shapeDagPath);
    }

    constexpr unsigned int wireframeDisplayStyles = (
        MHWRender::MFrameContext::DisplayStyle::kWireFrame |
        MHWRender::MFrameContext::DisplayStyle::kBoundingBox);

    const bool wireframeStyle = (displayStyle & wireframeDisplayStyles);

    const bool isActive =
        (displayStatus == MHWRender::DisplayStatus::kActive) ||
        (displayStatus == MHWRender::DisplayStatus::kHilite) ||
        (displayStatus == MHWRender::DisplayStatus::kActiveTemplate) ||
        (displayStatus == MHWRender::DisplayStatus::kActiveComponent) ||
        (displayStatus == MHWRender::DisplayStatus::kLead);

    if (wireframeStyle || isActive) {
        useWireframeColor = true;
    }

    return useWireframeColor;
}

/* static */
bool
PxrMayaHdShapeAdapter::_GetVisibility(
        const MDagPath& dagPath,
        const M3dView* view,
        bool* visibility)
{
    MStatus status;
    const MHWRender::DisplayStatus displayStatus =
        MHWRender::MGeometryUtilities::displayStatus(dagPath, &status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (displayStatus == MHWRender::kInvisible) {
        *visibility = false;
        return true;
    }

    // The displayStatus() method above does not account for things like
    // display layers, so we also check the shape's dag path for its visibility
    // state.
    const bool dagPathIsVisible = dagPath.isVisible(&status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (!dagPathIsVisible) {
        *visibility = false;
        return true;
    }

    // If a view was provided, check to see whether it is being filtered, and
    // get its isolated objects if so.
    MSelectionList isolatedObjects;
#if MAYA_API_VERSION >= 201700
    if (view && view->viewIsFiltered()) {
        view->filteredObjectList(isolatedObjects);
    }
#endif

    // If non-empty, isolatedObjects contains the "root" isolated objects, so
    // we'll need to check to see if one of our ancestors was isolated. (The
    // ancestor check is potentially slow if you're isolating selection in
    // a very large scene.)
    // If empty, nothing is being isolated. (You don't pay the cost of any
    // ancestor checking in this case.)
    const bool somethingIsolated = !isolatedObjects.isEmpty(&status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (somethingIsolated) {
        bool isIsolateVisible = false;
        MDagPath curPath(dagPath);
        while (curPath.length()) {
            const bool hasItem = isolatedObjects.hasItem(
                    curPath, MObject::kNullObj, &status);
            if (status != MS::kSuccess) {
                return false;
            }
            if (hasItem) {
                isIsolateVisible = true;
                break;
            }
            curPath.pop();
        }
        *visibility = isIsolateVisible;
        return true;
    }

    // Passed all visibility checks.
    *visibility = true;
    return true;
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


PXR_NAMESPACE_CLOSE_SCOPE
