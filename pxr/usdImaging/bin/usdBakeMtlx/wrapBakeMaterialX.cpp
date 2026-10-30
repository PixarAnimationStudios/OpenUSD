//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/bin/usdBakeMtlx/bakeMaterialX.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/pyNanobindBridge.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdBakeMtlx(nb::module_ &m)
{
    m.def("BakeMaterial", UsdBakeMtlxBakeMaterial, 
          nb::arg("mtlxMaterial"),
          nb::arg("bakedMtlxDir"),
          nb::arg("textureWidth"),
          nb::arg("textureHeight"),
          nb::arg("bakeHdr"),
          nb::arg("bakeAverage"));
    m.def("ReadFileToStage", UsdBakeMtlxReadDocToStage,
          nb::arg("pathname"), nb::arg("stage"));
}
