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
#include "usdMaya/MayaCameraWriter.h"

#include "usdMaya/JobArgs.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>

PXR_NAMESPACE_OPEN_SCOPE



MayaCameraWriter::MayaCameraWriter(const MDagPath & iDag, const SdfPath& uPath, usdWriteJobCtx& jobCtx) :
    MayaTransformWriter(iDag, uPath, false, jobCtx) // cameras are not instanced
{
    UsdGeomCamera primSchema =
        UsdGeomCamera::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

/* virtual */
void MayaCameraWriter::write(const UsdTimeCode &usdTime)
{
    // == Write
    UsdGeomCamera primSchema(mUsdPrim);

    // Write parent class attrs
    writeTransformAttrs(usdTime, primSchema);

    // Write the attrs
    writeCameraAttrs(usdTime, primSchema);
}

/* virtual */
bool MayaCameraWriter::writeCameraAttrs(const UsdTimeCode &usdTime, UsdGeomCamera &primSchema)
{
    // Since write() above will take care of any animation on the camera's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == isShapeAnimated()) {
        return true;
    }

    MStatus status;

    MFnCamera camFn(getDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // NOTE: We do not use a GfCamera and then call SetFromCamera() below
    // because we want the xformOps populated by the parent class to survive.
    // Using SetFromCamera() would stomp them with a single "transform" xformOp.

    // Set the type of projection.
    if (camFn.isOrtho()) {
        primSchema.GetProjectionAttr().Set(UsdGeomTokens->orthographic, usdTime);
    } else {
        primSchema.GetProjectionAttr().Set(UsdGeomTokens->perspective, usdTime);
    }

    // Setup the aperture.
    primSchema.GetHorizontalApertureAttr().Set(
        float(PxrUsdMayaUtil::ConvertInchesToMM(
                camFn.horizontalFilmAperture() *
                camFn.lensSqueezeRatio())),
        usdTime);
    primSchema.GetVerticalApertureAttr().Set(
        float(PxrUsdMayaUtil::ConvertInchesToMM(
                camFn.verticalFilmAperture() *
                camFn.lensSqueezeRatio())),
        usdTime);

    primSchema.GetHorizontalApertureOffsetAttr().Set(
        float(camFn.horizontalFilmOffset()), usdTime);
    primSchema.GetVerticalApertureOffsetAttr().Set(
        float(camFn.verticalFilmOffset()), usdTime);

    // Set the lens parameters.
    primSchema.GetFocalLengthAttr().Set(
        float(camFn.focalLength()), usdTime);

    // Always export focus distance and fStop regardless of what
    // camFn.isDepthOfField() says. Downstream tools can choose to ignore or
    // override them.
    primSchema.GetFocusDistanceAttr().Set(
        float(camFn.focusDistance()), usdTime);
    primSchema.GetFStopAttr().Set(
        float(camFn.fStop()), usdTime);

    // Set the clipping planes.
    GfVec2f clippingRange(camFn.nearClippingPlane(), camFn.farClippingPlane());
    primSchema.GetClippingRangeAttr().Set(clippingRange, usdTime);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

