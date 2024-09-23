//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static object 
_GetBaseNameAndType(const TfToken &fullName)
{
    const auto &result = UsdShadeUtils::GetBaseNameAndType(fullName);
    return make_tuple(result.first, result.second);
}

} // anonymous namespace 

void wrapUsdShadeUtils()
{
    UsdShadeAttributeVector (*GetValueProducingAttributes_Input)(
        const UsdShadeInput &input, bool includeAuthoredValues) = 
            &UsdShadeUtils::GetValueProducingAttributes;
    UsdShadeAttributeVector (*GetValueProducingAttributes_Output)(
        const UsdShadeOutput &output, bool includeAuthoredValues) = 
            &UsdShadeUtils::GetValueProducingAttributes;

    scope thisScope = class_<UsdShadeUtils>("Utils", no_init)
        .def("GetPrefixForAttributeType", 
            UsdShadeUtils::GetPrefixForAttributeType)
        .staticmethod("GetPrefixForAttributeType")

        .def("GetConnectedSourcePath", 
             UsdShadeUtils::GetConnectedSourcePath, 
             (arg("connectionSourceInfo")))
        .staticmethod("GetConnectedSourcePath")

        .def("GetBaseNameAndType", _GetBaseNameAndType)
        .staticmethod("GetBaseNameAndType")

        .def("GetType", UsdShadeUtils::GetType)
        .staticmethod("GetType")

        .def("GetFullName", UsdShadeUtils::GetFullName)
        .staticmethod("GetFullName")

        .def("GetValueProducingAttributes", GetValueProducingAttributes_Input,
            (arg("input"), arg("shaderOutputsOnly")=false))
        .def("GetValueProducingAttributes", GetValueProducingAttributes_Output,
            (arg("output"), arg("shaderOutputsOnly")=false))
        .staticmethod("GetValueProducingAttributes")
        ;

}
