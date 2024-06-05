//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>

#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python/return_by_value.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapPipeline()
{
    def("GetAlphaAttributeNameForColor", UsdUtilsGetAlphaAttributeNameForColor, arg("colorAttrName"));
    def("GetModelNameFromRootLayer", UsdUtilsGetModelNameFromRootLayer);
    def("GetRegisteredVariantSets", 
            UsdUtilsGetRegisteredVariantSets, 
            return_value_policy<TfPySequenceToList>());
    def("GetPrimAtPathWithForwarding", UsdUtilsGetPrimAtPathWithForwarding, 
        (arg("stage"), arg("path")));
    def("UninstancePrimAtPath", UsdUtilsUninstancePrimAtPath, 
        (arg("stage"), arg("path")));
    def("GetPrimaryUVSetName", UsdUtilsGetPrimaryUVSetName,
        return_value_policy<return_by_value>());
    def("GetPrefName", UsdUtilsGetPrefName,
        return_value_policy<return_by_value>());
    def(
        "GetMaterialsScopeName",
        UsdUtilsGetMaterialsScopeName,
        arg("forceDefault")=false);
    def(
        "GetPrimaryCameraName",
        UsdUtilsGetPrimaryCameraName,
        arg("forceDefault")=false);
}
