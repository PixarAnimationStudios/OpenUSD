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
#include "usdKatana/attrMap.h"
#include "usdKatana/readCamera.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/camera.h"
#include "pxr/imaging/cameraUtil/screenWindowParameters.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <FnAttribute/FnDataBuilder.h>
#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadCamera");

void
PxrUsdKatanaReadCamera(
        const UsdGeomCamera& camera,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

    //
    // Set all general attributes for a xformable type.
    //

    PxrUsdKatanaReadXformable(camera, data, attrs);

    // want both "type" and "bound" to stomp
    attrs.set("type", FnKat::StringAttribute("camera"));

    // Cameras do not have bounding boxes, but we won't return an empty bbox
    // because Katana/PRMan will not behave well.
    // Catching the request for a "bound" attribute here prevents the bound
    // computation from returning an empty bound, which is treated as a fail
    attrs.set("bound", FnKat::Attribute());

    const bool camerasAreZup = UsdUtilsGetCamerasAreZup(data.GetUsdInArgs()->GetStage());

    const GfCamera cam = camera.GetCamera(currentTime, camerasAreZup);

    //
    // Set the 'prmanGlobalStatements.camera.depthOfField' attribute.
    //

    FnKat::GroupBuilder pgsBuilder;
    FnKat::GroupBuilder cameraBuilder;
    FnKat::GroupBuilder dofBuilder;

    const double fStop = cam.GetFStop();

    if (fStop == 0.0) {
        dofBuilder.set("fStopInfinite", FnKat::StringAttribute("Yes"));
    } else {
        dofBuilder.set("fStopInfinite", FnKat::StringAttribute("No"));

        // GfCamera's focalLength is in mm, Renderman's in cm,
        // convert here.
        const double focalLength =
            cam.GetFocalLength() * GfCamera::FOCAL_LENGTH_UNIT;
        const double focusDistance = cam.GetFocusDistance();

        // Write unmodified fStop to Renderman. This gives the correct
        // result with RIS.
        // (Historically, we were multiplying the fStop by
        //     filmbackWidth (in cm) * lensSqueeze / 2
        // see CalculateDepthOfField and _CalculateFStopAdjustment in
        // change 1047654)
        dofBuilder.set("fStop",     FnKat::FloatAttribute(fStop));
        dofBuilder.set("focalLen",  FnKat::FloatAttribute(focalLength));
        dofBuilder.set("focalDist", FnKat::FloatAttribute(focusDistance));
    }

    cameraBuilder.set("depthOfField", dofBuilder.build());
    pgsBuilder.set("camera", cameraBuilder.build());

    attrs.set("prmanGlobalStatements", pgsBuilder.build());

    //
    // Set the 'geometry' attribute.
    //

    FnKat::GroupBuilder geoBuilder;

    const CameraUtilScreenWindowParameters params(cam);
    GfVec4d screenWindow = params.GetScreenWindow();
    
    if (cam.GetProjection() == GfCamera::Perspective)
    {
        geoBuilder.set("projection",
                       FnKat::StringAttribute("perspective"));

        //
        // Check to see if the focal length attribute is animated.
        // If so, emit motion samples for the camera FOV.
        //

        UsdAttribute focalLengthAttr = camera.GetFocalLengthAttr();

        bool isVarying = 
            PxrUsdKatanaUtils::IsAttributeVarying(
                focalLengthAttr, currentTime);

        const std::vector<double>& motionSampleTimes =
            data.GetMotionSampleTimes(camera.GetFocalLengthAttr());

        const bool isMotionBackward = data.GetUsdInArgs()->IsMotionBackward();

        FnKat::DoubleBuilder fovBuilder(1);
        TF_FOR_ALL(iter, motionSampleTimes)
        {
            double relSampleTime = *iter;
            double time = currentTime + relSampleTime;

            double fov = camera.GetCamera(time, camerasAreZup
                ).GetFieldOfView(GfCamera::FOVHorizontal);

            fovBuilder.push_back(fov, isMotionBackward ?
                PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

            if (!isVarying)
            {
                break;
            }
        }

        geoBuilder.set("fov", fovBuilder.build());
    }
    else
    {
        geoBuilder.set("projection",
                       FnKat::StringAttribute("orthographic"));
        // Always write out fov.
        // XXX - Katana barfs on a missing fov for ortho cams and considers it a 
        // malformed camera (even though it's ignored by prman). So let's go
        // ahead and set one for now (it's ignored anyway).
        geoBuilder.set("fov",
                       FnKat::DoubleAttribute(70.0));
        
        // Katana only appears to work correctly if the screenwindow has
        // width 2.0 and the orthographicWidth is the actual
        // orthographicWidth, so rescale.
        const double orthographicWidth = screenWindow[1] - screenWindow[0];
        geoBuilder.set("orthographicWidth",
                       FnKat::DoubleAttribute(orthographicWidth));
        screenWindow /= orthographicWidth / 2.0;
    }
    
    geoBuilder.set("left",
                   FnKat::DoubleAttribute(screenWindow[0]));
    geoBuilder.set("right",
                   FnKat::DoubleAttribute(screenWindow[1]));
    geoBuilder.set("bottom",
                   FnKat::DoubleAttribute(screenWindow[2]));
    geoBuilder.set("top",
                   FnKat::DoubleAttribute(screenWindow[3]));
    geoBuilder.set("near",
                   FnKat::DoubleAttribute(cam.GetClippingRange().GetMin()));
    geoBuilder.set("far",
                   FnKat::DoubleAttribute(cam.GetClippingRange().GetMax()));
    
    // Katana expresses clipping planes via a worldspace transformation
    // (as a location predeclared at /root/world).
    // The USD values are a normal and a distance from the
    // camera back. Transfer the values literally here and we'll deal with
    // the transformation in a downstream Op.
    std::vector<GfVec4f> clippingPlanes = cam.GetClippingPlanes();
    if (!clippingPlanes.empty())
    {
        geoBuilder.set("usdClippingPlanes", FnAttribute::FloatAttribute(
                &clippingPlanes[0][0], clippingPlanes.size() * 4, 4));
    }

    // XXX The camera's zUp needs to be recorded until we have no
    // more USD z-Up assets and the katana assets have no more prerotate
    // camera nodes.
    geoBuilder.set("isZUp", FnKat::IntAttribute(camerasAreZup ? 1 : 0));
    
    attrs.set("geometry", geoBuilder.build());
}

PXR_NAMESPACE_CLOSE_SCOPE

