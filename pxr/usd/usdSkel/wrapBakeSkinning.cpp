//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/bakeSkinning.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE


void wrapUsdSkelBakeSkinning(nb::module_ &m)
{
    m.def("BakeSkinning",
          (bool (*)(const UsdSkelRoot&, const GfInterval&))&UsdSkelBakeSkinning,
          nb::arg("root"), nb::arg("interval")=GfInterval::GetFullInterval());

    m.def("BakeSkinning",
          (bool (*)(const UsdPrimRange&, const GfInterval&))&UsdSkelBakeSkinning,
          nb::arg("range"), nb::arg("interval")=GfInterval::GetFullInterval());
}
