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
#include "pxr/pxr.h"
#include "pxrUsdMayaGL/proxyDrawOverride.h"

#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "usdMaya/proxyShape.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MSelectionContext.h>
#include <maya/MSelectionMask.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


MString UsdMayaProxyDrawOverride::sm_drawDbClassification("drawdb/geometry/usdMaya");
MString UsdMayaProxyDrawOverride::sm_drawRegistrantId("pxrUsdPlugin");

/* static */
MHWRender::MPxDrawOverride*
UsdMayaProxyDrawOverride::Creator(const MObject& obj)
{
    UsdMayaGLBatchRenderer::Init();
    return new UsdMayaProxyDrawOverride(obj);
}

// Note that isAlwaysDirty became available as an MPxDrawOverride constructor
// parameter beginning with Maya 2016 Extension 2.
UsdMayaProxyDrawOverride::UsdMayaProxyDrawOverride(const MObject& obj) :
        MHWRender::MPxDrawOverride(obj,
                                   UsdMayaProxyDrawOverride::draw
#if MAYA_API_VERSION >= 201651
                                   , /* isAlwaysDirty = */ false)
#else
                                   )
#endif
{
}

/* virtual */
UsdMayaProxyDrawOverride::~UsdMayaProxyDrawOverride()
{
    UsdMayaGLBatchRenderer::GetInstance().RemoveShapeAdapter(&_shapeAdapter);
}

/* virtual */
MHWRender::DrawAPI
UsdMayaProxyDrawOverride::supportedDrawAPIs() const
{
#if MAYA_API_VERSION >= 201600
    return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile;
#else
    return MHWRender::kOpenGL;
#endif
}

/* virtual */
MBoundingBox
UsdMayaProxyDrawOverride::boundingBox(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */) const
{
    UsdMayaProxyShape* pShape = UsdMayaProxyShape::GetShapeAtDagPath(objPath);
    if (!pShape) {
        return MBoundingBox();
    }

    return pShape->boundingBox();
}

/* virtual */
bool
UsdMayaProxyDrawOverride::isBounded(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */) const
{
    return UsdMayaIsBoundingBoxModeEnabled();
}

/* virtual */
MUserData*
UsdMayaProxyDrawOverride::prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */,
        const MHWRender::MFrameContext& frameContext,
        MUserData* oldData)
{
    UsdMayaProxyShape* shape = UsdMayaProxyShape::GetShapeAtDagPath(objPath);
    if (!shape) {
        return nullptr;
    }

    if (!_shapeAdapter.Sync(
            shape,
            frameContext.getDisplayStyle(),
            MHWRender::MGeometryUtilities::displayStatus(objPath))) {
        return nullptr;
    }

    UsdMayaGLBatchRenderer::GetInstance().AddShapeAdapter(&_shapeAdapter);

    bool drawShape;
    bool drawBoundingBox;
    PxrMayaHdRenderParams params =
        _shapeAdapter.GetRenderParams(&drawShape, &drawBoundingBox);

    if (!drawBoundingBox && !drawShape) {
        // We weren't asked to do anything.
        return nullptr;
    }

    MBoundingBox bounds;
    MBoundingBox* boundsPtr = nullptr;
    if (drawBoundingBox) {
        // Only query for the bounding box if we're drawing it.
        bounds = shape->boundingBox();
        boundsPtr = &bounds;
    }

    return UsdMayaGLBatchRenderer::GetInstance().CreateBatchDrawData(
        oldData,
        params,
        drawShape,
        boundsPtr);
}

#if MAYA_API_VERSION >= 201800

/* virtual */
bool
UsdMayaProxyDrawOverride::wantUserSelection() const
{
    const MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        return false;
    }

    return renderer->drawAPIIsOpenGL();
}

/* virtual */
bool
UsdMayaProxyDrawOverride::userSelect(
        MHWRender::MSelectionInfo& selectInfo,
        const MHWRender::MDrawContext& context,
        MPoint& hitPoint,
        const MUserData* data)
{
    MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);
    if (!selectInfo.selectable(objectsMask)) {
        return false;
    }

    UsdMayaProxyShape* shape =
        UsdMayaProxyShape::GetShapeAtDagPath(_shapeAdapter._shapeDagPath);
    if (!shape) {
        return false;
    }

    const unsigned int displayStyle = context.getDisplayStyle();
    const MHWRender::DisplayStatus displayStatus =
        MHWRender::MGeometryUtilities::displayStatus(_shapeAdapter._shapeDagPath);

    // At this point, we expect the shape to have already been drawn and our
    // shape adapter to have been added to the batch renderer, but just in
    // case, we still treat the shape adapter as if we're populating it for the
    // first time. We do not add it to the batch renderer though, since that
    // must have already been done to have caused the shape to be drawn and
    // become eligible for selection.
    if (!_shapeAdapter.Sync(shape, displayStyle, displayStatus)) {
        return false;
    }

    GfVec3f batchHitPoint;
    const bool didHit =
        UsdMayaGLBatchRenderer::GetInstance().TestIntersection(
            &_shapeAdapter,
            selectInfo,
            context,
            selectInfo.singleSelection(),
            &batchHitPoint);

    if (didHit) {
        hitPoint = MPoint(batchHitPoint[0], batchHitPoint[1], batchHitPoint[2]);
    }

    return didHit;
}

#endif // MAYA_API_VERSION >= 201800

/* static */
void
UsdMayaProxyDrawOverride::draw(
        const MHWRender::MDrawContext& context,
        const MUserData* data)
{
    UsdMayaGLBatchRenderer::GetInstance().Draw(context, data);
}


PXR_NAMESPACE_CLOSE_SCOPE
