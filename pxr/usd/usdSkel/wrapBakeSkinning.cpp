//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/bakeSkinning.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"

#include "pxr/external/boost/python.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


void wrapUsdSkelBakeSkinning()
{
    def("BakeSkinning", ((bool (*)(const UsdSkelRoot&,
                                   const GfInterval&))&UsdSkelBakeSkinning),
        (arg("root"), arg("interval")=GfInterval::GetFullInterval()));

    def("BakeSkinning", ((bool (*)(const UsdPrimRange&,
                                   const GfInterval&))&UsdSkelBakeSkinning),
        (arg("range"), arg("interval")=GfInterval::GetFullInterval()));

}
