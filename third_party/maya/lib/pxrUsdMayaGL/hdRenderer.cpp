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

// Make sure to include glew first before any header that might include
// gl.h
#include "pxr/imaging/glf/glew.h"

#include "pxrUsdMayaGL/hdRenderer.h"
#include "px_vp20/utils.h"
#include "px_vp20/utils_legacy.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMaterial.h>
#include <maya/MMatrix.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectInfo.h>
#include <maya/MStateManager.h>
#include <maya/MViewport2Renderer.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


void
UsdMayaGLHdRenderer::CheckRendererSetup(
        const UsdPrim& usdPrim,
        const SdfPathVector& excludePaths)
{
    if (usdPrim != _renderedPrim || excludePaths != _excludePrimPaths) {
        _renderedPrim = usdPrim;
        _excludePrimPaths = excludePaths;

        _renderer.reset(new UsdImagingGL(_renderedPrim.GetPath(),
                                         _excludePrimPaths));
    }
}

void UsdMayaGLHdRenderer::GenerateDefaultVp2DrawRequests(
    const MDagPath& objPath,
    const MHWRender::MFrameContext& frameContext,
    const MBoundingBox& bounds,
    UsdMayaGLHdRenderer::RequestDataArray *requestArray
    )
{
    if(requestArray == NULL) {
        return;
    }
    M3dView viewHelper = M3dView::active3dView();

    const MHWRender::DisplayStatus displayStatus =
        MHWRender::MGeometryUtilities::displayStatus(objPath);

    const bool isSelected =
            (displayStatus == MHWRender::kActive) ||
            (displayStatus == MHWRender::kLead)   ||
            (displayStatus == MHWRender::kHilite);

    const MColor mayaWireframeColor =
        MHWRender::MGeometryUtilities::wireframeColor(objPath);
    const GfVec4f wireframeColor(mayaWireframeColor.r,
                                 mayaWireframeColor.g,
                                 mayaWireframeColor.b,
                                 mayaWireframeColor.a);


    requestArray->clear();

    if( !(frameContext.getDisplayStyle() & MHWRender::MFrameContext::DisplayStyle::kWireFrame) &&
        !(frameContext.getDisplayStyle() & MHWRender::MFrameContext::DisplayStyle::kBoundingBox) )
    {
        UsdMayaGLHdRenderer::RequestData shadedRequest;
        shadedRequest.fWireframeColor = wireframeColor;
        shadedRequest.bounds = bounds;
// Maya 2015 lacks MHWRender::MFrameContext::DisplayStyle::kFlatShaded for whatever reason...
#if MAYA_API_VERSION >= 201600
        if( frameContext.getDisplayStyle() & MHWRender::MFrameContext::DisplayStyle::kFlatShaded )
        {
            shadedRequest.drawRequest.setToken( UsdMayaGLHdRenderer::DRAW_SHADED_FLAT );
            shadedRequest.drawRequest.setDisplayStyle(M3dView::kFlatShaded);
        }
        else
#endif
        {
            shadedRequest.drawRequest.setToken( UsdMayaGLHdRenderer::DRAW_SHADED_SMOOTH );
            shadedRequest.drawRequest.setDisplayStyle(M3dView::kGouraudShaded);
        }

        requestArray->push_back( shadedRequest );
    }

    if(isSelected || (frameContext.getDisplayStyle() & MHWRender::MFrameContext::DisplayStyle::kWireFrame) )
    {
        UsdMayaGLHdRenderer::RequestData wireRequest;
        wireRequest.bounds = bounds;
        wireRequest.drawRequest.setToken( UsdMayaGLHdRenderer::DRAW_WIREFRAME);
        wireRequest.drawRequest.setDisplayStyle(M3dView::kWireFrame);
        wireRequest.fWireframeColor = wireframeColor;
        requestArray->push_back( wireRequest );
    }
}

void UsdMayaGLHdRenderer::RenderVp2(
    const RequestDataArray &requests,
    const MHWRender::MDrawContext& context,
    UsdImagingGLRenderParams params) const
{
    using namespace MHWRender;

    MStatus status;
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    MHWRender::MStateManager* stateMgr = context.getStateManager();
    if (!stateMgr) return;

    const int displayStyle = context.getDisplayStyle();
    if (displayStyle == 0) return;

    if (displayStyle & MDrawContext::kXray) {
        // Viewport 2.0 will call draw() twice when drawing transparent objects
        // (X-Ray mode). We skip the first draw() call.
        const MRasterizerState* rasterState = stateMgr->getRasterizerState();
        if (rasterState && rasterState->desc().cullMode == MRasterizerState::kCullFront) {
            return;
        }
    }

    if (!theRenderer->drawAPIIsOpenGL()) return;


    glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT);

    const MMatrix worldView =
        context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx, &status);
    GfMatrix4d modelViewMatrix(worldView.matrix);

    const MMatrix projection =
        context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
    GfMatrix4d projectionMatrix(projection.matrix);

    // get root matrix
    MMatrix root = context.getMatrix(MHWRender::MDrawContext::kWorldMtx, &status);
    GfMatrix4d rootMatrix(root.matrix);

    // Extract camera settings from maya view
    int viewX, viewY, viewWidth, viewHeight;
    context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);

    GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

    M3dView::DisplayStyle viewDisplayStyle = displayStyle & MDrawContext::kWireFrame ?
        M3dView::kWireFrame : M3dView::kGouraudShaded;

    if(viewDisplayStyle == M3dView::kGouraudShaded)
    {
        px_vp20Utils::setupLightingGL(context);
        glEnable(GL_LIGHTING);
    }

    _renderer->SetCameraState(modelViewMatrix, projectionMatrix, viewport);

    _renderer->SetLightingStateFromOpenGL();


    TF_FOR_ALL(it, requests) {
        RequestData request = *it;
        if(viewDisplayStyle == M3dView::kWireFrame && request.drawRequest.displayStyle() == M3dView::kGouraudShaded) {
            request.drawRequest.setDisplayStyle(viewDisplayStyle);
        }

        switch(request.drawRequest.token()) {
        case UsdMayaGLHdRenderer::DRAW_WIREFRAME:
        case UsdMayaGLHdRenderer::DRAW_POINTS: {

            params.drawMode = request.drawRequest.token() == 
                UsdMayaGLHdRenderer::DRAW_WIREFRAME ? 
                    UsdImagingGLDrawMode::DRAW_WIREFRAME :
                    UsdImagingGLDrawMode::DRAW_POINTS;
            params.enableLighting = false;
            params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_NOTHING;

            params.overrideColor = request.fWireframeColor;

            // Get and render usdPrim
            _renderer->Render(_renderedPrim, params);

            break;
        }
        case UsdMayaGLHdRenderer::DRAW_SHADED_FLAT:
        case UsdMayaGLHdRenderer::DRAW_SHADED_SMOOTH: {


            params.drawMode = ((request.drawRequest.token() == 
                    UsdMayaGLHdRenderer::DRAW_SHADED_FLAT) ?
                        UsdImagingGLDrawMode::DRAW_GEOM_FLAT : 
                        UsdImagingGLDrawMode::DRAW_GEOM_SMOOTH);
            params.enableLighting = true;
            params.cullStyle = 
                UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;

            _renderer->Render(_renderedPrim, params);


            break;
        }
        case UsdMayaGLHdRenderer::DRAW_BOUNDING_BOX: {
            px_vp20Utils::RenderBoundingBox(request.bounds,
                                            request.fWireframeColor,
                                            worldView,
                                            projection);
            break;
        }
        }
    }

    if(displayStyle == M3dView::kGouraudShaded)
    {
        px_vp20Utils::unsetLightingGL(context);
    }

    glPopAttrib(); // GL_CURRENT_BIT | GL_LIGHTING_BIT
}

void
UsdMayaGLHdRenderer::Render(
        const MDrawRequest& request,
        M3dView& view,
        UsdImagingGLRenderParams params) const
{
    if (!_renderedPrim.IsValid()) {
        return;
    }
    view.beginGL();

    // Extract camera settings from maya view
    MMatrix mayaViewMatrix;
    MMatrix mayaProjMatrix;
    unsigned int viewX, viewY, viewWidth, viewHeight;

    // Have to pull out as MMatrix
    view.modelViewMatrix(mayaViewMatrix);
    view.projectionMatrix(mayaProjMatrix);
    view.viewport(viewX, viewY, viewWidth, viewHeight);
    // Convert MMatrix to GfMatrix. It's a shame because
    // the memory layout is identical
    GfMatrix4d modelViewMatrix(mayaViewMatrix.matrix);
    GfMatrix4d projectionMatrix(mayaProjMatrix.matrix);
    GfVec4d viewport(viewX, viewY, viewWidth, viewHeight);

    _renderer->SetCameraState(modelViewMatrix, projectionMatrix, viewport);
    _renderer->SetLightingStateFromOpenGL();


    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    glEnable(GL_LIGHTING);

    int drawMode = request.token();
    switch (drawMode) {
        case DRAW_WIREFRAME:
        case DRAW_POINTS: {


            params.drawMode = drawMode == DRAW_WIREFRAME ? 
                UsdImagingGLDrawMode::DRAW_WIREFRAME : 
                UsdImagingGLDrawMode::DRAW_POINTS;
            params.enableLighting = false;
            glGetFloatv(GL_CURRENT_COLOR, &params.overrideColor[0]);

            // Get and render usdPrim
            _renderer->Render(_renderedPrim, params);


            break;
        }
        case DRAW_SHADED_FLAT:
        case DRAW_SHADED_SMOOTH: {

            //
            // setup the material
            //



            params.drawMode = drawMode == DRAW_SHADED_FLAT ?
                UsdImagingGLDrawMode::DRAW_SHADED_FLAT : 
                UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;

            _renderer->Render(_renderedPrim, params);

            break;
        }
        case DRAW_BOUNDING_BOX: {
            MDrawData drawData = request.drawData();
            const MPxSurfaceShape* shape =
                static_cast<const MPxSurfaceShape*>(drawData.geometry());

            if (!shape) {
                break;
            }
            if (!shape->isBounded()) {
                break;
            }

            const MBoundingBox bbox = shape->boundingBox();
            const MColor mayaColor = request.color();
            const GfVec4f wireframeColor(mayaColor.r,
                                         mayaColor.g,
                                         mayaColor.b,
                                         mayaColor.a);

            px_vp20Utils::RenderBoundingBox(bbox,
                                            wireframeColor,
                                            mayaViewMatrix,
                                            mayaProjMatrix);

            break;
        }
    }

    glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    glPopAttrib(); // GL_ENABLE_BIT | GL_CURRENT_BIT
    view.endGL();
}

bool
UsdMayaGLHdRenderer::TestIntersection(
        MSelectInfo& selectInfo,
        UsdImagingGLRenderParams params,
        GfVec3d* hitPoint) const
{
    // Guard against user clicking in viewer before renderer is setup
    if (!_renderer) {
        return false;
    }

    if (!_renderedPrim.IsValid()) {
        return false;
    }

    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;
    px_LegacyViewportUtils::GetSelectionMatrices(
        selectInfo,
        viewMatrix,
        projectionMatrix);

    params.drawMode = UsdImagingGLDrawMode::DRAW_GEOM_ONLY;

    return _renderer->TestIntersection(viewMatrix,
                                       projectionMatrix,
                                       GfMatrix4d().SetIdentity(),
                                       _renderedPrim,
                                       params,
                                       hitPoint);
}

/* static */
float
UsdMayaGLHdRenderer::SubdLevelToComplexity(int subdLevel)
{
    // Here is how to map subdivision level to the RenderParameter complexity
    // It is done this way for historical reasons
    //
    // For complexity->subdLevel:
    //   (int)(TfMax(0.0f,TfMin(1.0f,complexity-1.0f))*5.0f+0.1f);
    //
    // complexity usd
    //    1.0      0
    //    1.1      1
    //    1.2      2
    //    1.3      3
    //    1.4      3  (not 4, because of floating point precision)
    //    1.5      5
    //    1.6      6
    //    1.7      7
    //    1.8      8
    //    1.9      8
    //    2.0      8
    //
    return 1.0+(float(subdLevel)*0.1f);
}


PXR_NAMESPACE_CLOSE_SCOPE

