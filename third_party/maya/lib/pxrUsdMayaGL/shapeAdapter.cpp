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

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Synchronizing PxrMayaHdShapeAdapter for Viewport 2.0: %p\n",
        this);

    return _Sync(shapeDagPath, displayStyle, displayStatus);
}

/* virtual */
bool
PxrMayaHdShapeAdapter::UpdateVisibility()
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

    // If we're in this method, we must be prepping for a legacy viewport
    // render, so mark a legacy render as pending.
    UsdMayaGLBatchRenderer::GetInstance()._UpdateLegacyRenderPending(true);

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

    newData->drawShape = _drawShape;

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
