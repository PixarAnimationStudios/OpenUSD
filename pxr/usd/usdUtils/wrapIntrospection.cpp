//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/usdUtils/introspection.h"

#include "pxr/base/tf/pyStaticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

VtDictionary 
_WrapUsdUtilsComputeUsdStageStats_1(const std::string &rootLayerPath) {
    VtDictionary stats;
    UsdUtilsComputeUsdStageStats(rootLayerPath, &stats);
    return stats;    
}

VtDictionary 
_WrapUsdUtilsComputeUsdStageStats_2(const UsdStageWeakPtr &stage) {
    VtDictionary stats;
    UsdUtilsComputeUsdStageStats(stage, &stats);
    return stats;
}

} // anonymous namespace 

void wrapIntrospection(nanobind::module_ &m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(m, "UsdStageStatsKeys", UsdUtilsUsdStageStatsKeys, 
                             USDUTILS_USDSTAGE_STATS);

    m.def("ComputeUsdStageStats", _WrapUsdUtilsComputeUsdStageStats_1);
    m.def("ComputeUsdStageStats", _WrapUsdUtilsComputeUsdStageStats_2);
}
