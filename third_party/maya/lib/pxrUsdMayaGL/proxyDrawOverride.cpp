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

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MString.h>
#include <maya/MUserData.h>


PXR_NAMESPACE_OPEN_SCOPE


MString UsdMayaProxyDrawOverride::sm_drawDbClassification("drawdb/geometry/usdMaya");
MString UsdMayaProxyDrawOverride::sm_drawRegistrantId("pxrUsdPlugin");


UsdMayaProxyDrawOverride::UsdMayaProxyDrawOverride(const MObject& obj) :
        MHWRender::MPxDrawOverride(obj, UsdMayaProxyDrawOverride::draw)
{
}

UsdMayaProxyDrawOverride::~UsdMayaProxyDrawOverride()
{
}

/* static */
MHWRender::MPxDrawOverride*
UsdMayaProxyDrawOverride::Creator(const MObject& obj)
{
    UsdMayaGLBatchRenderer::Init();
    return new UsdMayaProxyDrawOverride(obj);
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
MUserData*
UsdMayaProxyDrawOverride::prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */,
        const MHWRender::MFrameContext& frameContext,
        MUserData* userData)
{
    UsdMayaProxyShape* shape = UsdMayaProxyShape::GetShapeAtDagPath(objPath);
    if (!shape) {
        return nullptr;
    }

    // The shape adapter is owned by the global batch renderer, which is a
    // singleton.
    const UsdPrim usdPrim = shape->usdPrim();
    const SdfPathVector excludedPrimPaths = shape->getExcludePrimPaths();
    PxrMayaHdShapeAdapter* shapeAdapter =
        UsdMayaGLBatchRenderer::GetInstance().GetShapeAdapter(objPath,
                                                              usdPrim,
                                                              excludedPrimPaths);

    if (!shapeAdapter->Sync(
            shape,
            frameContext.getDisplayStyle(),
            MHWRender::MGeometryUtilities::displayStatus(objPath))) {
        return nullptr;
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
            userData,
            params,
            drawShape,
            &bounds);
    }
    //
    // Like above but with no bounding box...
    else if (drawShape) {
        UsdMayaGLBatchRenderer::GetInstance().QueueShapeForDraw(
            shapeAdapter,
            userData,
            params,
            drawShape,
            nullptr);
    }
    else {
        // we weren't asked to do anything.
        return nullptr;
    }

    return userData;
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

/* static */
void
UsdMayaProxyDrawOverride::draw(
        const MHWRender::MDrawContext& context,
        const MUserData* data)
{
    UsdMayaGLBatchRenderer::GetInstance().Draw(context, data);
}


PXR_NAMESPACE_CLOSE_SCOPE
