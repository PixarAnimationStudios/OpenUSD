//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/usdUtils/localizeAsset.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapLocalizeAsset(nanobind::module_ &m)
{
    m.def("LocalizeAsset", UsdUtilsLocalizeAsset,
            nb::arg("assetPath"),
            nb::arg("localizationDirectory"),
            nb::arg("editLayersInPlace") = false,
            nb::arg("processingFunc").none() = nb::none());
}
