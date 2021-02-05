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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
