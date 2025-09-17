//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "writer.h"

#include "pxr/pxr.h"

#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdDraco()
{
    def("_WriteDraco", UsdDraco_WriteDraco,
        (arg("mesh"),
         arg("fileName"),
         arg("qp"),
         arg("qt"),
         arg("qn"),
         arg("cl"),
         arg("preservePolygons"),
         arg("preservePositionOrder"),
         arg("preserveHoles")));
    def("_PrimvarSupported", UsdDraco_PrimvarSupported,
        (arg("primvar")));
}
