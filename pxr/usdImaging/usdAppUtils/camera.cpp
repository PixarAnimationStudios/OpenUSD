//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/camera.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/camera.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdGeomCamera
UsdAppUtilsGetCameraAtPath(
        const UsdStagePtr& stage,
        const SdfPath& cameraPath)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCamera();
    }

    if (!cameraPath.IsPrimPath()) {
        // A non-prim path cannot be a camera.
        return UsdGeomCamera();
    }

    SdfPath usdCameraPath = cameraPath;

    if (!cameraPath.IsAbsolutePath()) {
        if (cameraPath.GetPathElementCount() > 1u) {
            // XXX: Perhaps we should error here? For now we coerce the camera
            // path to be absolute using the absolute root path and print a
            // warning.
            usdCameraPath =
                cameraPath.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
            TF_WARN(
                "Camera path \"%s\" is not absolute. Using absolute path "
                "instead: \"%s\"",
                cameraPath.GetText(),
                usdCameraPath.GetText());
        } else {
            // Search for the camera by name.
            UsdPrimRange primRange =
                UsdPrimRange::Stage(stage, UsdTraverseInstanceProxies());

            for (const UsdPrim& usdPrim : primRange) {
                if (usdPrim.GetName() == cameraPath.GetNameToken()) {
                    const UsdGeomCamera usdCamera(usdPrim);
                    if (usdCamera) {
                        return usdCamera;
                    }
                }
            }
        }
    }

    return UsdGeomCamera::Get(stage, usdCameraPath);
}


PXR_NAMESPACE_CLOSE_SCOPE
