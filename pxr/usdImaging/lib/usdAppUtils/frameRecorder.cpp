//
// Copyright 2019 Pixar
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

// Must be included before GL headers.
#include "pxr/imaging/glf/glew.h"

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/frameRecorder.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


UsdAppUtilsFrameRecorder::UsdAppUtilsFrameRecorder() :
    _imageWidth(960u),
    _complexity(1.0f),
    _colorCorrectionMode("disabled"),
    _purposes({UsdGeomTokens->default_, UsdGeomTokens->proxy})
{
    GlfGlewInit();
}

static bool
_HasPurpose(const TfTokenVector& purposes, const TfToken& purpose)
{
    return std::find(purposes.begin(), purposes.end(), purpose) != purposes.end();
}

void
UsdAppUtilsFrameRecorder::SetIncludedPurposes(const TfTokenVector& purposes) 
{
    TfTokenVector  allPurposes = { UsdGeomTokens->render,
                                   UsdGeomTokens->proxy,
                                   UsdGeomTokens->guide };
    _purposes = { UsdGeomTokens->default_ };

    for ( TfToken const &p : purposes) {
        if (_HasPurpose(allPurposes, p)){
            _purposes.push_back(p);
        }
        else if (p != UsdGeomTokens->default_) {
            // We allow "default" to be specified even though
            // it's unnecessary
            TF_CODING_ERROR("Unrecognized purpose value '%s'.",
                            p.GetText());
        }
    }
}

static GfCamera
_ComputeCameraToFrameStage(const UsdStagePtr& stage, UsdTimeCode timeCode,
                           const TfTokenVector& includedPurposes)
{
    // Start with a default (50mm) perspective GfCamera.
    GfCamera gfCamera;
    UsdGeomBBoxCache bboxCache(timeCode, includedPurposes,
                               /* useExtentsHint = */ true);
    GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
    GfVec3d center = bbox.ComputeCentroid();
    GfRange3d range = bbox.ComputeAlignedRange();
    GfVec3d dim = range.GetSize();
    TfToken upAxis = UsdGeomGetStageUpAxis(stage);
    // Find corner of bbox in the focal plane.
    GfVec2d plane_corner;
    if (upAxis == UsdGeomTokens->y) {
        plane_corner = GfVec2d(dim[0], dim[1])/2;
    } else {
        plane_corner = GfVec2d(dim[0], dim[2])/2;
    }
    float plane_radius = sqrt(GfDot(plane_corner, plane_corner));
    // Compute distance to focal plane.
    float half_fov = gfCamera.GetFieldOfView(GfCamera::FOVHorizontal)/2.0;
    float distance = plane_radius / tan(GfDegreesToRadians(half_fov));
    // Back up to frame the front face of the bbox.
    if (upAxis == UsdGeomTokens->y) {
        distance += dim[2] / 2;
    } else {
        distance += dim[1] / 2;
    }
    // Compute local-to-world transform for camera filmback.
    GfMatrix4d xf;
    if (upAxis == UsdGeomTokens->y) {
        xf.SetTranslate(center + GfVec3d(0, 0, distance));
    } else {
        xf.SetRotate(GfRotation(GfVec3d(1,0,0), 90));
        xf.SetTranslateOnly(center + GfVec3d(0, -distance, 0));
    }
    gfCamera.SetTransform(xf);
    return gfCamera;
}

bool
UsdAppUtilsFrameRecorder::Record(
        const UsdStagePtr& stage,
        const UsdGeomCamera& usdCamera,
        const UsdTimeCode timeCode,
        const std::string& outputImagePath)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return false;
    }

    if (outputImagePath.empty()) {
        TF_CODING_ERROR("Invalid empty output image path");
        return false;
    }

    const GfVec4f CLEAR_COLOR(0.0f);
    const GfVec4f SCENE_AMBIENT(0.01f, 0.01f, 0.01f, 1.0f);
    const GfVec4f SPECULAR_DEFAULT(0.1f, 0.1f, 0.1f, 1.0f);
    const GfVec4f AMBIENT_DEFAULT(0.2f, 0.2f, 0.2f, 1.0f);
    const float   SHININESS_DEFAULT(32.0);
    
    // XXX: If the camera's aspect ratio is animated, then a range of calls to
    // this function may generate a sequence of images with different sizes.
    GfCamera gfCamera;
    if (usdCamera) {
        gfCamera = usdCamera.GetCamera(timeCode);
    } else {
        gfCamera = _ComputeCameraToFrameStage(stage, timeCode, _purposes);
    }
    float aspectRatio = gfCamera.GetAspectRatio();
    if (GfIsClose(aspectRatio, 0.0f, 1e-4)) {
        aspectRatio = 1.0f;
    }

    const size_t imageHeight = std::max<size_t>(
        static_cast<size_t>(static_cast<float>(_imageWidth) / aspectRatio),
        1u);
    const GfVec2i renderResolution(_imageWidth, imageHeight);

    const GfFrustum frustum = gfCamera.GetFrustum();
    const GfVec3d cameraPos = frustum.GetPosition();

    _imagingEngine.SetCameraState(
        frustum.ComputeViewMatrix(),
        frustum.ComputeProjectionMatrix());
    _imagingEngine.SetRenderViewport(
        GfVec4d(
            0.0,
            0.0,
            static_cast<double>(_imageWidth),
            static_cast<double>(imageHeight)));

    GlfSimpleLight cameraLight(
        GfVec4f(cameraPos[0], cameraPos[1], cameraPos[2], 1.0f));
    cameraLight.SetAmbient(SCENE_AMBIENT);

    const GlfSimpleLightVector lights({cameraLight});

    // Make default material and lighting match usdview's defaults... we expect 
    // GlfSimpleMaterial to go away soon, so not worth refactoring for sharing
    GlfSimpleMaterial material;
    material.SetAmbient(AMBIENT_DEFAULT);
    material.SetSpecular(SPECULAR_DEFAULT);
    material.SetShininess(SHININESS_DEFAULT);

    _imagingEngine.SetLightingState(lights, material, SCENE_AMBIENT);

    UsdImagingGLRenderParams renderParams;
    renderParams.frame = timeCode;
    renderParams.complexity = _complexity;
    renderParams.colorCorrectionMode = _colorCorrectionMode;
    renderParams.clearColor = CLEAR_COLOR;
    renderParams.renderResolution = renderResolution;
    renderParams.showProxy = _HasPurpose(_purposes, UsdGeomTokens->proxy);
    renderParams.showRender = _HasPurpose(_purposes, UsdGeomTokens->render);
    renderParams.showGuides = _HasPurpose(_purposes, UsdGeomTokens->guide);
    
    glEnable(GL_DEPTH_TEST);

    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(renderResolution);
    drawTarget->Bind();

    drawTarget->AddAttachment("color",
        GL_RGBA, GL_FLOAT, GL_RGBA);
    drawTarget->AddAttachment("depth",
        GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);

    glViewport(0, 0, _imageWidth, imageHeight);

    const GLfloat CLEAR_DEPTH[1] = { 1.0f };
    const UsdPrim& pseudoRoot = stage->GetPseudoRoot();

    do {
        glClearBufferfv(GL_COLOR, 0, CLEAR_COLOR.data());
        glClearBufferfv(GL_DEPTH, 0, CLEAR_DEPTH);
        _imagingEngine.Render(pseudoRoot, renderParams);
    } while (!_imagingEngine.IsConverged());

    drawTarget->Unbind();

    return drawTarget->WriteToFile("color", outputImagePath);
}


PXR_NAMESPACE_CLOSE_SCOPE
