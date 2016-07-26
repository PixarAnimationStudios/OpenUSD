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
#include "usdMaya/proxyShapeUI.h"
#include "usdMaya/proxyShape.h"

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

void*
UsdMayaProxyShapeUI::creator()
{
    UsdMayaGLBatchRenderer::Init();
    return new UsdMayaProxyShapeUI();
}

void
UsdMayaProxyShapeUI::getDrawRequests(
    const MDrawInfo& drawInfo,
    bool /* isObjectAndActiveOnly */,
    MDrawRequestQueue& requests)
{
    MDrawRequest request = drawInfo.getPrototype(*this);
    
    UsdMayaGLBatchRenderer::ShapeRenderer* shapeRenderer =
        _GetShapeRenderer( drawInfo.multiPath(), /*prepareForQueue= */ true );
    if( !shapeRenderer )
        return;

    bool drawShape, drawBoundingBox;
    UsdMayaGLBatchRenderer::RenderParams params =
        shapeRenderer->GetRenderParams(
                            drawInfo.multiPath(),
                            drawInfo.displayStyle(),
                            drawInfo.displayStatus(),
                            &drawShape,
                            &drawBoundingBox );

    // Only query bounds if we're drawing bounds...
    //
    if( drawBoundingBox )
    {
        UsdMayaProxyShape* shape = 
            static_cast<UsdMayaProxyShape*>(surfaceShape());
        
        MBoundingBox bounds;
        bounds = shape->boundingBox();
        
        // Note that drawShape is still passed through here.
        shapeRenderer->QueueShapeForDraw(
            this, request, params, drawShape, &bounds );
    }
    //
    // Like above but with no bounding box...
    else if( drawShape )
    {
        shapeRenderer->QueueShapeForDraw(
            this, request, params, drawShape, NULL );
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

void
UsdMayaProxyShapeUI::draw(
    const MDrawRequest& request, 
    M3dView& view) const
{
    view.beginGL();
    
    UsdMayaGLBatchRenderer::GetGlobalRenderer().Draw( request, view );
    
    view.endGL();
}

bool
UsdMayaProxyShapeUI::select(
    MSelectInfo& selectInfo,
    MSelectionList& selectionList,
    MPointArray& worldSpaceSelectedPoints) const
{
    MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);
    // selectable() takes MSelectionMask&, not const MSelectionMask.  :(.
    if( !selectInfo.selectable(objectsMask) )
        return false;

    UsdMayaGLBatchRenderer::ShapeRenderer* shapeRenderer =
        _GetShapeRenderer( selectInfo.selectPath(), /*prepareForQueue= */ false );
    if( !shapeRenderer )
        return false;

    // object selection
    M3dView view = selectInfo.view();
    
    // We will miss very small objects with this setting, but it's faster.
    const unsigned int selectRes = 256;
    
    GfVec3d hitPoint;
    bool didHit = shapeRenderer->TestIntersection(
                        view, selectRes,
                        selectInfo.singleSelection(), &hitPoint ) ;

    if( didHit )
    {
        MSelectionList newSelectionList;
        newSelectionList.add(selectInfo.selectPath());

        MPoint mayaHitPoint;
        // Transform the hit point into the correct space
        // and make it a maya point
        mayaHitPoint = MPoint(hitPoint[0], hitPoint[1], hitPoint[2]);

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

UsdMayaGLBatchRenderer::ShapeRenderer*
UsdMayaProxyShapeUI::_GetShapeRenderer(
    const MDagPath &objPath,
    bool prepareForQueue ) const
{
    UsdMayaProxyShape* shape = 
        static_cast<UsdMayaProxyShape*>(surfaceShape());
    
    UsdPrim usdPrim;
    SdfPathVector excludePaths;
    UsdTimeCode timeCode;
    int subdLevel;
    bool showGuides, showRenderGuides;
    bool tint;
    GfVec4f tintColor;
    if( !shape->GetAllRenderAttributes(
                    &usdPrim, &excludePaths, &subdLevel, &timeCode,
                    &showGuides, &showRenderGuides,
                    &tint, &tintColor ) )
    {
        return NULL;
    }
    
    UsdMayaGLBatchRenderer::ShapeRenderer *outShapeRenderer =
        UsdMayaGLBatchRenderer::GetGlobalRenderer().GetShapeRenderer(
            usdPrim, excludePaths, objPath );
    
    if( prepareForQueue )
        outShapeRenderer->PrepareForQueue(
                objPath, timeCode, subdLevel, showGuides, showRenderGuides,
                tint, tintColor );

    return outShapeRenderer;
}

UsdMayaProxyShapeUI::UsdMayaProxyShapeUI()
    : MPxSurfaceShapeUI()
{
}

UsdMayaProxyShapeUI::~UsdMayaProxyShapeUI() {
    // empty
}
