//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "writer.h"

#include "pxr/pxr.h"

#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
