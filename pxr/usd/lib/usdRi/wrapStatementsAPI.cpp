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
#include "pxr/usd/usdRi/statementsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateFocusRegionAttr(UsdRiStatementsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFocusRegionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiStatementsAPI()
{
    typedef UsdRiStatementsAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("StatementsAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Apply", &This::Apply, (arg("stage"), arg("path")))
        .staticmethod("Apply")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetFocusRegionAttr",
             &This::GetFocusRegionAttr)
        .def("CreateFocusRegionAttr",
             &_CreateFocusRegionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

static SdfPathVector
_GetModelCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelCoordinateSystems(&result);
    return result;
}

static SdfPathVector
_GetModelScopedCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelScopedCoordinateSystems(&result);
    return result;
}

WRAP_CUSTOM {
    _class
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const TfType &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             (arg("name"), arg("tfType"), arg("nameSpace")="user"))
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const std::string &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             (arg("name"), arg("riType"), arg("nameSpace")="user"))
        .def("CreateRiAttributeAsRel", &UsdRiStatementsAPI::CreateRiAttributeAsRel,
             (arg("name"), arg("nameSpace")="user"))
        .def("GetRiAttributes", &UsdRiStatementsAPI::GetRiAttributes,
             (arg("nameSpace")=""),
             return_value_policy<TfPySequenceToList>())
        .def("GetRiAttributeName",
             UsdRiStatementsAPI::GetRiAttributeName, (arg("prop")))
        .staticmethod("GetRiAttributeName")
        .def("GetRiAttributeNameSpace",
             &UsdRiStatementsAPI::GetRiAttributeNameSpace, (arg("prop")))
        .staticmethod("GetRiAttributeNameSpace")
        .def("IsRiAttribute", &UsdRiStatementsAPI::IsRiAttribute, (arg("prop")))
        .staticmethod("IsRiAttribute")
        .def("MakeRiAttributePropertyName",
             &UsdRiStatementsAPI::MakeRiAttributePropertyName, (arg("attrName")))
        .staticmethod("MakeRiAttributePropertyName")
        .def("SetCoordinateSystem", &UsdRiStatementsAPI::SetCoordinateSystem,
             (arg("coordSysName")))
        .def("GetCoordinateSystem", &UsdRiStatementsAPI::GetCoordinateSystem)
        .def("HasCoordinateSystem", &UsdRiStatementsAPI::HasCoordinateSystem)

        .def("SetScopedCoordinateSystem",
             &UsdRiStatementsAPI::SetScopedCoordinateSystem,
             (arg("coordSysName")))
        .def("GetScopedCoordinateSystem",
             &UsdRiStatementsAPI::GetScopedCoordinateSystem)
        .def("HasScopedCoordinateSystem",
             &UsdRiStatementsAPI::HasScopedCoordinateSystem)

        .def("GetModelCoordinateSystems", _GetModelCoordinateSystems,
             return_value_policy<TfPySequenceToList>())
        .def("GetModelScopedCoordinateSystems", _GetModelScopedCoordinateSystems,
             return_value_policy<TfPySequenceToList>())
        ;
}

} // anonymous namespace
