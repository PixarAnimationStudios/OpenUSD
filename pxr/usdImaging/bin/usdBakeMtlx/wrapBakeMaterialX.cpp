//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/bin/usdBakeMtlx/bakeMaterialX.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/makePyConstructor.h"

#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdBakeMtlx()
{
    def("BakeMaterial", UsdBakeMtlxBakeMaterial, 
        ( arg("mtlxMaterial"),
          arg("bakedMtlxDir"),
          arg("textureWidth"),
          arg("textureHeight"),
          arg("bakeHdr"),
          arg("bakeAverage") ));
    def("ReadFileToStage", UsdBakeMtlxReadDocToStage,
        ( arg("pathname"), arg("stage") ),
        return_value_policy<TfPyRefPtrFactory<>>());

}
