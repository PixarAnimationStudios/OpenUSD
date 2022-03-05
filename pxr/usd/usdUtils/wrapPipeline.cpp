//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>

#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python/return_by_value.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

void wrapPipeline()
{
    boost::python::def("GetAlphaAttributeNameForColor", UsdUtilsGetAlphaAttributeNameForColor, boost::python::arg("colorAttrName"));
    boost::python::def("GetModelNameFromRootLayer", UsdUtilsGetModelNameFromRootLayer);
    boost::python::def("GetRegisteredVariantSets", 
            UsdUtilsGetRegisteredVariantSets, 
            boost::python::return_value_policy<TfPySequenceToList>());
    boost::python::def("GetPrimAtPathWithForwarding", UsdUtilsGetPrimAtPathWithForwarding, 
        (boost::python::arg("stage"), boost::python::arg("path")));
    boost::python::def("UninstancePrimAtPath", UsdUtilsUninstancePrimAtPath, 
        (boost::python::arg("stage"), boost::python::arg("path")));
    boost::python::def("GetPrimaryUVSetName", UsdUtilsGetPrimaryUVSetName,
        boost::python::return_value_policy<boost::python::return_by_value>());
    boost::python::def("GetPrefName", UsdUtilsGetPrefName,
        boost::python::return_value_policy<boost::python::return_by_value>());
    boost::python::def(
        "GetMaterialsScopeName",
        UsdUtilsGetMaterialsScopeName,
        boost::python::arg("forceDefault")=false);
    boost::python::def(
        "GetPrimaryCameraName",
        UsdUtilsGetPrimaryCameraName,
        boost::python::arg("forceDefault")=false);
}
