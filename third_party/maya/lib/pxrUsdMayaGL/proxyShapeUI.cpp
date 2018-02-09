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

#include "pxr/base/gf/vec3d.h"
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

static
PxrMayaHdShapeAdapter*
_GetShapeAdapter(
        UsdMayaProxyShape* shape,
        const MDagPath& objPath,
        const M3dView::DisplayStyle displayStyle,
        const M3dView::DisplayStatus displayStatus)
{
    if (!shape) {
        return nullptr;
    }

    // The shape adapter is owned by the global batch renderer, which is a
    // singleton.
    const UsdPrim usdPrim = shape->usdPrim();
    const SdfPathVector excludePaths = shape->getExcludePrimPaths();
    PxrMayaHdShapeAdapter* outShapeAdapter =
        UsdMayaGLBatchRenderer::GetInstance().GetShapeAdapter(objPath,
                                                              usdPrim,
                                                              excludePaths);
    if (!outShapeAdapter) {
        return nullptr;
    }

    if (!outShapeAdapter->Sync(shape, displayStyle, displayStatus)) {
        return nullptr;
    }

    return outShapeAdapter;
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

    PxrMayaHdShapeAdapter* shapeAdapter =
        _GetShapeAdapter(shape,
                         shapeDagPath,
                         drawInfo.displayStyle(),
                         drawInfo.displayStatus());
    if (!shapeAdapter) {
        return;
    }

    bool drawShape;
    bool drawBoundingBox;
    PxrMayaHdRenderParams params =
        shapeAdapter->GetRenderParams(&drawShape,
                                      &drawBoundingBox);

    // Only query bounds if we're drawing bounds...
    //
    if (drawBoundingBox) {
        const MBoundingBox bounds = shape->boundingBox();

        // Note that drawShape is still passed through here.
        UsdMayaGLBatchRenderer::GetInstance().QueueShapeForDraw(
            shapeAdapter,
            this,
            request,
            params,
            drawShape,
            &bounds);
    }
    //
    // Like above but with no bounding box...
    else if (drawShape) {
        UsdMayaGLBatchRenderer::GetInstance().QueueShapeForDraw(
            shapeAdapter,
            this,
            request,
            params,
            drawShape,
            nullptr);
    }
    else
    {
        // we weren't asked to do anything.
        return;
    }

    //
    // add the request to the queue
    //
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

    PxrMayaHdShapeAdapter* shapeAdapter =
        _GetShapeAdapter(shape,
                         selectInfo.selectPath(),
                         view.displayStyle(),
                         view.displayStatus(selectInfo.selectPath()));
    if (!shapeAdapter) {
        return false;
    }

    // object selection

    // We will miss very small objects with this setting, but it's faster.
    const unsigned int selectRes = 256;

    GfVec3d hitPoint;
    const bool didHit =
        UsdMayaGLBatchRenderer::GetInstance().TestIntersection(
            shapeAdapter,
            view,
            selectRes,
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
UsdMayaProxyShapeUI::~UsdMayaProxyShapeUI() {
    // empty
}


PXR_NAMESPACE_CLOSE_SCOPE
