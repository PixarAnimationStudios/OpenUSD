//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/camera.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/def.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


void
wrapCamera()
{
    def(
        "GetCameraAtPath",
        UsdAppUtilsGetCameraAtPath,
        (arg("stage"), arg("cameraPath")));
}
