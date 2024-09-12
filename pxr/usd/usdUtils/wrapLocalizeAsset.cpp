//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/usdUtils/localizeAsset.h"

#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapLocalizeAsset()
{
    def("LocalizeAsset", UsdUtilsLocalizeAsset,
            (arg("assetPath"),
             arg("localizationDirectory"),
             arg("editLayersInPlace") = false,
             arg("processingFunc") = object()));
}
