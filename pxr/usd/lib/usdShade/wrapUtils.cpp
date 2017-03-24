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

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

static object 
_GetBaseNameAndType(const TfToken &fullName)
{
    const auto &result = UsdShadeUtils::GetBaseNameAndType(fullName);
    return make_tuple(result.first, result.second);
}

void wrapUsdShadeUtils()
{
    enum_<UsdShadeAttributeType>("AttributeType")
        .value("Input", UsdShadeAttributeType::Input)
        .value("Output", UsdShadeAttributeType::Output)
        .value("Parameter", UsdShadeAttributeType::Parameter)
        .value("InterfaceAttribute", 
            UsdShadeAttributeType::InterfaceAttribute)
        ;

    scope thisScope = class_<UsdShadeUtils>("Utils", no_init)
        .def("GetPrefixForAttributeType", 
            UsdShadeUtils::GetPrefixForAttributeType)
        .staticmethod("GetPrefixForAttributeType")

        .def("GetBaseNameAndType", _GetBaseNameAndType)
        .staticmethod("GetBaseNameAndType")

        .def("GetFullName", UsdShadeUtils::GetFullName)
        .staticmethod("GetFullName")

        .def("WriteNewEncoding", UsdShadeUtils::WriteNewEncoding)
        .staticmethod("WriteNewEncoding")
        
        .def("ReadOldEncoding", UsdShadeUtils::ReadOldEncoding)
        .staticmethod("ReadOldEncoding")
    ;

}

PXR_NAMESPACE_CLOSE_SCOPE

