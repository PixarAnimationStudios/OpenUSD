//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_APP_UTILS_CAMERA_H
#define PXR_USD_IMAGING_USD_APP_UTILS_CAMERA_H

/// \file usdAppUtils/camera.h
///
/// Collection of module-scoped utilities for applications that operate using
/// USD cameras.

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/api.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/camera.h"


PXR_NAMESPACE_OPEN_SCOPE


/// Gets the UsdGeomCamera matching \p cameraPath from the USD stage \p stage.
///
/// If \p cameraPath is an absolute path, this is equivalent to
/// UsdGeomCamera::Get(). Otherwise, if \p cameraPath is a single-element path
/// representing just the name of a camera prim, then \p stage will be searched
/// looking for a UsdGeomCamera matching that name. The UsdGeomCamera schema
/// for that prim will be returned if found, or an invalid UsdGeomCamera will
/// be returned if not.
///
/// Note that if \p cameraPath is a multi-element path, a warning is issued and
/// it is just made absolute using the absolute root path before searching. In
/// the future, this could potentially be changed to use a suffix-based match.
USDAPPUTILS_API
UsdGeomCamera UsdAppUtilsGetCameraAtPath(
        const UsdStagePtr& stage,
        const SdfPath& cameraPath);


PXR_NAMESPACE_CLOSE_SCOPE


#endif
