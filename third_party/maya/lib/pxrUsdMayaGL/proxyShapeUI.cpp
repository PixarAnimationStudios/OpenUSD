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
#include "pxrUsdMayaGL/proxyShapeUI.h"

#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/renderParams.h"
#include "pxrUsdMayaGL/shapeAdapter.h"
#include "usdMaya/proxyShape.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionList.h>
#include <maya/MSelectionMask.h>


PXR_NAMESPACE_OPEN_SCOPE


/* static */
void*
UsdMayaProxyShapeUI::creator()
{
    UsdMayaGLBatchRenderer::Init();
    return new UsdMayaProxyShapeUI();
}

/* virtual */
void
UsdMayaProxyShapeUI::getDrawRequests(
        const MDrawInfo& drawInfo,
        bool /* isObjectAndActiveOnly */,
        MDrawRequestQueue& requests)
{
    MDrawRequest request = drawInfo.getPrototype(*this);

    const MDagPath shapeDagPath = drawInfo.multiPath();
    UsdMayaProxyShape* shape =
        UsdMayaProxyShape::GetShapeAtDagPath(shapeDagPath);
    if (!shape) {
        return;
    }

    if (!_shapeAdapter.Sync(shape,
                            drawInfo.displayStyle(),
                            drawInfo.displayStatus())) {
        return;
    }

    UsdMayaGLBatchRenderer::GetInstance().AddShapeAdapter(&_shapeAdapter);

    bool drawShape;
    bool drawBoundingBox;
    PxrMayaHdRenderParams params =
        _shapeAdapter.GetRenderParams(&drawShape, &drawBoundingBox);

    if (!drawBoundingBox && !drawShape) {
        // We weren't asked to do anything.
        return;
    }

    MBoundingBox bounds;
    MBoundingBox* boundsPtr = nullptr;
    if (drawBoundingBox) {
        // Only query for the bounding box if we're drawing it.
        bounds = shape->boundingBox();
        boundsPtr = &bounds;
    }

    UsdMayaGLBatchRenderer::GetInstance().CreateBatchDrawData(
        this,
        request,
        params,
        drawShape,
        boundsPtr);

    // Add the request to the queue.
    requests.add(request);
}

/* virtual */
void
UsdMayaProxyShapeUI::draw(const MDrawRequest& request, M3dView& view) const
{
    view.beginGL();

    UsdMayaGLBatchRenderer::GetInstance().Draw(request, view);

    view.endGL();
}

/* virtual */
bool
UsdMayaProxyShapeUI::select(
        MSelectInfo& selectInfo,
        MSelectionList& selectionList,
        MPointArray& worldSpaceSelectedPoints) const
{
    MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);

    // selectable() takes MSelectionMask&, not const MSelectionMask.  :(.
    if (!selectInfo.selectable(objectsMask)) {
        return false;
    }

    M3dView view = selectInfo.view();

    // Note that we cannot use UsdMayaProxyShape::GetShapeAtDagPath() here.
    // selectInfo.selectPath() returns the dag path to the assembly node, not
    // the shape node, so we don't have the shape node's path readily available.
    UsdMayaProxyShape* shape = static_cast<UsdMayaProxyShape*>(surfaceShape());
    if (!shape) {
        return false;
    }

    if (!_shapeAdapter.Sync(shape,
                            view.displayStyle(),
                            view.displayStatus(selectInfo.selectPath()))) {
        return false;
    }

    GfVec3f hitPoint;
    const bool didHit =
        UsdMayaGLBatchRenderer::GetInstance().TestIntersection(
            &_shapeAdapter,
            view,
            selectInfo.singleSelection(),
            &hitPoint);

    if (didHit) {
        MSelectionList newSelectionList;
        newSelectionList.add(selectInfo.selectPath());

        MPoint mayaHitPoint = MPoint(hitPoint[0], hitPoint[1], hitPoint[2]);

        selectInfo.addSelection(
            newSelectionList,
            mayaHitPoint,
            selectionList,
            worldSpaceSelectedPoints,

            // even though this is an "object", we use the "meshes" selection
            // mask here.  This allows us to select usd assemblies that are
            // switched to "full" as well as those that are still collapsed.
            MSelectionMask(MSelectionMask::kSelectMeshes),

            false);
    }

    return didHit;
}

UsdMayaProxyShapeUI::UsdMayaProxyShapeUI() : MPxSurfaceShapeUI()
{
}

/* virtual */
UsdMayaProxyShapeUI::~UsdMayaProxyShapeUI()
{
    UsdMayaGLBatchRenderer::GetInstance().RemoveShapeAdapter(&_shapeAdapter);
}


PXR_NAMESPACE_CLOSE_SCOPE
