//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/camera.h"

#include <boost/python.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;


PXR_NAMESPACE_USING_DIRECTIVE


void
wrapCamera()
{
    def(
        "GetCameraAtPath",
        UsdAppUtilsGetCameraAtPath,
        (arg("stage"), arg("cameraPath")));
}
